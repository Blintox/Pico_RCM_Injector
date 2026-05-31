#pragma once

#include <stdint.h>

/*
 * Example placeholder payload header.
 *
 * Generate the real file with:
 *   python tools/bin2payload_h.py path/to/payload.bin include/payload.h
 */
static const uint8_t rcm_payload[] = { 0x00 };
static const uint32_t rcm_payload_len = 0;
