// SPDX-License-Identifier: GPL-3.0-only
//
// Pico/TinyUSB host port of the RCM launch sequence used by public Fusee
// launcher implementations. This does not include or fetch any payload binary.

#include "rcm_injector.h"

#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"

enum {
    APX_VID = 0x0955,
    APX_PID = 0x7321,
    APX_EP_OUT = 0x01,
    APX_EP_IN = 0x81,
    APX_EP_SIZE = 64,
    PACKET_CHUNK_SIZE = 0x1000,
    TRIGGER_READ_SIZE = 0x7000,
    INTERMEZZO_SIZE = 92,
};

typedef struct {
    uint8_t dev_addr;
    uint8_t *buffer;
    size_t used;
    uint32_t packets_written;
} buffered_writer_t;

typedef struct {
    volatile bool complete;
    volatile xfer_result_t result;
    volatile uint32_t actual_len;
} xfer_waiter_t;

static const uint8_t intermezzo[INTERMEZZO_SIZE] = {
    0x44, 0x00, 0x9F, 0xE5, 0x01, 0x11, 0xA0, 0xE3, 0x40, 0x20, 0x9F, 0xE5, 0x00, 0x20, 0x42, 0xE0,
    0x08, 0x00, 0x00, 0xEB, 0x01, 0x01, 0xA0, 0xE3, 0x10, 0xFF, 0x2F, 0xE1, 0x00, 0x00, 0xA0, 0xE1,
    0x2C, 0x00, 0x9F, 0xE5, 0x2C, 0x10, 0x9F, 0xE5, 0x02, 0x28, 0xA0, 0xE3, 0x01, 0x00, 0x00, 0xEB,
    0x20, 0x00, 0x9F, 0xE5, 0x10, 0xFF, 0x2F, 0xE1, 0x04, 0x30, 0x90, 0xE4, 0x04, 0x30, 0x81, 0xE4,
    0x04, 0x20, 0x52, 0xE2, 0xFB, 0xFF, 0xFF, 0x1A, 0x1E, 0xFF, 0x2F, 0xE1, 0x20, 0xF0, 0x01, 0x40,
    0x5C, 0xF0, 0x01, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x01, 0x40,
};

static CFG_TUH_MEM_SECTION CFG_TUH_MEM_ALIGN uint8_t chunk_buffer[PACKET_CHUNK_SIZE];
static CFG_TUH_MEM_SECTION CFG_TUH_MEM_ALIGN uint8_t trigger_buffer[TRIGGER_READ_SIZE];

static const tusb_desc_endpoint_t apx_ep_out_desc = {
    .bLength = sizeof(tusb_desc_endpoint_t),
    .bDescriptorType = TUSB_DESC_ENDPOINT,
    .bEndpointAddress = APX_EP_OUT,
    .bmAttributes = { .xfer = TUSB_XFER_BULK },
    .wMaxPacketSize = APX_EP_SIZE,
    .bInterval = 0,
};

static const tusb_desc_endpoint_t apx_ep_in_desc = {
    .bLength = sizeof(tusb_desc_endpoint_t),
    .bDescriptorType = TUSB_DESC_ENDPOINT,
    .bEndpointAddress = APX_EP_IN,
    .bmAttributes = { .xfer = TUSB_XFER_BULK },
    .wMaxPacketSize = APX_EP_SIZE,
    .bInterval = 0,
};

static void xfer_complete_cb(tuh_xfer_t *xfer) {
    xfer_waiter_t *waiter = (xfer_waiter_t *)xfer->user_data;
    waiter->result = xfer->result;
    waiter->actual_len = xfer->actual_len;
    waiter->complete = true;
}

static xfer_result_t wait_for_xfer(xfer_waiter_t *waiter, uint32_t timeout_ms) {
    const uint32_t started_ms = board_millis();

    while (!waiter->complete) {
        tuh_task();

        if (board_millis() - started_ms > timeout_ms) {
            return XFER_RESULT_TIMEOUT;
        }
    }

    return waiter->result;
}

static xfer_result_t edpt_xfer_sync(uint8_t dev_addr, uint8_t ep_addr, uint8_t *buffer, uint32_t len, uint32_t timeout_ms) {
    xfer_waiter_t waiter = {
        .complete = false,
        .result = XFER_RESULT_INVALID,
        .actual_len = 0,
    };

    tuh_xfer_t xfer = {
        .daddr = dev_addr,
        .ep_addr = ep_addr,
        .result = XFER_RESULT_INVALID,
        .buflen = len,
        .buffer = buffer,
        .complete_cb = xfer_complete_cb,
        .user_data = (uintptr_t)&waiter,
    };

    if (!tuh_edpt_xfer(&xfer)) {
        return XFER_RESULT_FAILED;
    }

    return wait_for_xfer(&waiter, timeout_ms);
}

static xfer_result_t control_xfer_sync(uint8_t dev_addr, tusb_control_request_t const *request, uint8_t *buffer, uint32_t timeout_ms) {
    xfer_waiter_t waiter = {
        .complete = false,
        .result = XFER_RESULT_INVALID,
        .actual_len = 0,
    };

    tuh_xfer_t xfer = {
        .daddr = dev_addr,
        .ep_addr = 0,
        .result = XFER_RESULT_INVALID,
        .setup = request,
        .buffer = buffer,
        .complete_cb = xfer_complete_cb,
        .user_data = (uintptr_t)&waiter,
    };

    if (!tuh_control_xfer(&xfer)) {
        return XFER_RESULT_FAILED;
    }

    return wait_for_xfer(&waiter, timeout_ms);
}

bool rcm_is_apx_device(uint8_t dev_addr) {
    uint16_t vid = 0;
    uint16_t pid = 0;

    return tuh_vid_pid_get(dev_addr, &vid, &pid) && vid == APX_VID && pid == APX_PID;
}

static bool open_apx_endpoints(uint8_t dev_addr) {
    return tuh_edpt_open(dev_addr, &apx_ep_out_desc) && tuh_edpt_open(dev_addr, &apx_ep_in_desc);
}

static bool read_device_id(uint8_t dev_addr, uint8_t device_id[16]) {
    memset(device_id, 0, 16);
    return edpt_xfer_sync(dev_addr, APX_EP_IN, device_id, 16, 1000) == XFER_RESULT_SUCCESS;
}

static bool writer_flush(buffered_writer_t *writer) {
    if (writer->used < PACKET_CHUNK_SIZE) {
        memset(writer->buffer + writer->used, 0, PACKET_CHUNK_SIZE - writer->used);
    }

    xfer_result_t result = edpt_xfer_sync(writer->dev_addr, APX_EP_OUT, writer->buffer, PACKET_CHUNK_SIZE, 2000);
    memset(writer->buffer, 0, PACKET_CHUNK_SIZE);
    writer->used = 0;

    if (result != XFER_RESULT_SUCCESS) {
        return false;
    }

    writer->packets_written++;
    return true;
}

static bool writer_write(buffered_writer_t *writer, const uint8_t *data, size_t len) {
    while (writer->used + len >= PACKET_CHUNK_SIZE) {
        size_t bytes_to_write = PACKET_CHUNK_SIZE - writer->used;
        memcpy(writer->buffer + writer->used, data, bytes_to_write);
        writer->used += bytes_to_write;

        if (!writer_flush(writer)) {
            return false;
        }

        data += bytes_to_write;
        len -= bytes_to_write;
    }

    if (len > 0) {
        memcpy(writer->buffer + writer->used, data, len);
        writer->used += len;
    }

    return true;
}

static bool writer_write_u32(buffered_writer_t *writer, uint32_t value) {
    const uint8_t bytes[4] = {
        (uint8_t)(value & 0xffu),
        (uint8_t)((value >> 8) & 0xffu),
        (uint8_t)((value >> 16) & 0xffu),
        (uint8_t)((value >> 24) & 0xffu),
    };

    return writer_write(writer, bytes, sizeof(bytes));
}

static bool writer_write_zeroes(buffered_writer_t *writer, size_t len) {
    static const uint8_t zeroes[64] = { 0 };

    while (len > 0) {
        size_t batch = len > sizeof(zeroes) ? sizeof(zeroes) : len;
        if (!writer_write(writer, zeroes, batch)) {
            return false;
        }
        len -= batch;
    }

    return true;
}

static bool build_and_send_payload(buffered_writer_t *writer, const uint8_t *payload, size_t payload_len) {
    if (!writer_write_u32(writer, 0x30298)) {
        return false;
    }

    if (!writer_write_zeroes(writer, 680 - 4)) {
        return false;
    }

    for (uint32_t i = 0; i < 0x3c00; i++) {
        if (!writer_write_u32(writer, 0x4001f000)) {
            return false;
        }
    }

    if (!writer_write(writer, intermezzo, sizeof(intermezzo))) {
        return false;
    }

    if (!writer_write_zeroes(writer, 0xfa4)) {
        return false;
    }

    if (!writer_write(writer, payload, payload_len)) {
        return false;
    }

    return writer_flush(writer);
}

static bool trigger_launch(uint8_t dev_addr) {
    memset(trigger_buffer, 0, sizeof(trigger_buffer));

    tusb_control_request_t request = {
        .bmRequestType = 0x81,
        .bRequest = TUSB_REQ_GET_STATUS,
        .wValue = 0,
        .wIndex = 0,
        .wLength = TRIGGER_READ_SIZE,
    };

    return control_xfer_sync(dev_addr, &request, trigger_buffer, 5000) == XFER_RESULT_SUCCESS;
}

rcm_inject_result_t rcm_inject_payload(uint8_t dev_addr, const uint8_t *payload, size_t payload_len) {
    if (payload == NULL || payload_len == 0) {
        return RCM_INJECT_ERR_NO_PAYLOAD;
    }

    if (!open_apx_endpoints(dev_addr)) {
        return RCM_INJECT_ERR_ENDPOINT_OPEN;
    }

    uint8_t device_id[16];
    if (read_device_id(dev_addr, device_id)) {
        printf("APX device ID:");
        for (size_t i = 0; i < sizeof(device_id); i++) {
            printf(" %02x", device_id[i]);
        }
        printf("\n");
    } else {
        printf("APX device ID read failed; continuing with payload transfer\n");
    }

    buffered_writer_t writer = {
        .dev_addr = dev_addr,
        .buffer = chunk_buffer,
        .used = 0,
        .packets_written = 0,
    };

    if (!build_and_send_payload(&writer, payload, payload_len)) {
        return RCM_INJECT_ERR_BULK_WRITE;
    }

    if ((writer.packets_written % 2u) != 1u && !writer_flush(&writer)) {
        return RCM_INJECT_ERR_BULK_WRITE;
    }

    if (!trigger_launch(dev_addr)) {
        return RCM_INJECT_ERR_TRIGGER;
    }

    return RCM_INJECT_OK;
}

const char *rcm_inject_result_name(rcm_inject_result_t result) {
    switch (result) {
    case RCM_INJECT_OK:
        return "ok";
    case RCM_INJECT_ERR_NO_PAYLOAD:
        return "no payload";
    case RCM_INJECT_ERR_ENDPOINT_OPEN:
        return "endpoint open failed";
    case RCM_INJECT_ERR_DEVICE_ID:
        return "device ID read failed";
    case RCM_INJECT_ERR_BULK_WRITE:
        return "bulk write failed";
    case RCM_INJECT_ERR_TRIGGER:
        return "trigger failed";
    default:
        return "unknown";
    }
}
