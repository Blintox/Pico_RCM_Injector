// SPDX-License-Identifier: GPL-3.0-only

#include <stdio.h>

#include "bsp/board_api.h"
#include "payload.h"
#include "pico/stdlib.h"
#include "rcm_injector.h"
#include "tusb.h"

typedef enum {
    APP_WAITING = 0,
    APP_INJECTING,
    APP_DONE,
    APP_ERROR,
} app_state_t;

static volatile uint8_t apx_dev_addr = 0;
static volatile bool apx_ready = false;
static app_state_t app_state = APP_WAITING;
static uint8_t error_blink_count = 0;

static void set_led(bool on) {
    board_led_write(on);
}

static void blink(unsigned count, unsigned on_ms, unsigned off_ms) {
    for (unsigned i = 0; i < count; i++) {
        set_led(true);
        sleep_ms(on_ms);
        set_led(false);
        sleep_ms(off_ms);
    }
}

static void status_task(void) {
    static uint32_t last_ms = 0;
    static uint32_t pause_start_ms = 0;
    static bool led = false;
    static uint8_t pulses_done = 0;
    const uint32_t now = board_millis();

    if (app_state == APP_WAITING) {
        if (now - last_ms < 500) {
            return;
        }

        last_ms = now;
        led = !led;
        set_led(led);
        return;
    }

    if (app_state != APP_ERROR || error_blink_count == 0) {
        return;
    }

    if (pulses_done >= error_blink_count) {
        set_led(false);

        if (pause_start_ms == 0) {
            pause_start_ms = now;
        }

        if (now - pause_start_ms >= 1000) {
            pause_start_ms = 0;
            pulses_done = 0;
            last_ms = now;
        }
        return;
    }

    if (now - last_ms >= 120) {
        last_ms = now;
        led = !led;
        set_led(led);

        if (!led) {
            pulses_done++;
        }
    }
}

void tuh_mount_cb(uint8_t dev_addr) {
    if (rcm_is_apx_device(dev_addr)) {
        apx_dev_addr = dev_addr;
        apx_ready = true;
    }
}

void tuh_umount_cb(uint8_t dev_addr) {
    if (apx_dev_addr == dev_addr) {
        apx_dev_addr = 0;
        apx_ready = false;
        app_state = APP_WAITING;
        error_blink_count = 0;
    }
}

int main(void) {
    stdio_init_all();
    board_init();

    printf("Pico RCM injector starting\n");
    printf("Payload size: %lu bytes\n", (unsigned long)rcm_payload_len);

    if (!tuh_init(BOARD_TUH_RHPORT)) {
        printf("TinyUSB host init failed\n");
        app_state = APP_ERROR;
    }

    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    blink(2, 75, 75);

    while (true) {
        tuh_task();
        status_task();

        if (app_state == APP_WAITING && apx_ready) {
            app_state = APP_INJECTING;
            set_led(true);
            sleep_ms(100);

            rcm_inject_result_t result = rcm_inject_payload(apx_dev_addr, rcm_payload, rcm_payload_len);
            printf("Injection result: %s\n", rcm_inject_result_name(result));

            if (result == RCM_INJECT_OK) {
                app_state = APP_DONE;
                blink(1, 250, 100);
            } else {
                app_state = APP_ERROR;
                error_blink_count = (uint8_t)result;
            }

            set_led(false);
        }
    }
}
