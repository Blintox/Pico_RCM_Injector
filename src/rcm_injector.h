#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    RCM_INJECT_OK = 0,
    RCM_INJECT_ERR_NO_PAYLOAD,
    RCM_INJECT_ERR_ENDPOINT_OPEN,
    RCM_INJECT_ERR_DEVICE_ID,
    RCM_INJECT_ERR_BULK_WRITE,
    RCM_INJECT_ERR_TRIGGER,
} rcm_inject_result_t;

bool rcm_is_apx_device(uint8_t dev_addr);
rcm_inject_result_t rcm_inject_payload(uint8_t dev_addr, const uint8_t *payload, size_t payload_len);
const char *rcm_inject_result_name(rcm_inject_result_t result);
