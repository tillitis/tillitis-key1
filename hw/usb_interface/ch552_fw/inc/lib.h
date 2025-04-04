// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#ifndef __LIB_H__
#define __LIB_H__

#include <stdint.h>

#include "config.h"

#ifdef USE_NUM_U8
int8_t uint8_to_str(uint8_t *buf, uint8_t bufsize, uint8_t n);
#endif

#ifdef USE_NUM_U32
int8_t uint32_to_str(uint8_t *buf, uint8_t bufsize, uint32_t n);
#endif

//uint8_t hex_to_uint8(const char *hex, uint8_t len);
//uint16_t hex_to_uint16(const char *hex, uint8_t len);
uint32_t hex_to_uint32(const char *hex, uint8_t len);

uint8_t ascii_hex_char_to_byte(uint8_t c);
//int ascii_hex_string_to_bytes(uint8_t *hex_str, uint8_t *out_bytes, size_t out_len);

#endif
