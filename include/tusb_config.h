#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined by the Pico SDK build.
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#ifndef CFG_TUH_MEM_SECTION
#define CFG_TUH_MEM_SECTION
#endif

#ifndef CFG_TUH_MEM_ALIGN
#define CFG_TUH_MEM_ALIGN __attribute__((aligned(4)))
#endif

#define CFG_TUH_ENABLED 1
#define CFG_TUH_DEVICE_MAX 1
#define CFG_TUH_HUB 0

#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT 0
#endif

#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif

#define CFG_TUH_MAX_SPEED BOARD_TUH_MAX_SPEED
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_ENDPOINT_MAX 4
#define CFG_TUH_API_EDPT_XFER 1

#define CFG_TUH_CDC 0
#define CFG_TUH_HID 0
#define CFG_TUH_MSC 0
#define CFG_TUH_VENDOR 0

#ifdef __cplusplus
}
#endif
