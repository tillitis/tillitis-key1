// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "io.h"
#include "lib.h"
#include "mem.h"
#include "print.h"

void printStr(uint8_t *str)
{
#ifdef USE_DEBUG_PRINT
#if defined(DEBUG_PRINT_HW)
    while (*str != 0) {
        CH554UART0SendByte(*str);
        ++str;
    }
#elif defined(DEBUG_PRINT_SW)
    uint32_t str_len = strlen(str);
    CH554UART1SendByte(IO_CH552);
    CH554UART1SendByte(str_len);
    CH554UART1SendBuffer(str, str_len);
#endif
#else
    (void)str;
#endif
}

void printChar(uint8_t c)
{
#ifdef USE_DEBUG_PRINT
#if defined(DEBUG_PRINT_HW)
    CH554UART0SendByte(c);
#elif defined(DEBUG_PRINT_SW)
    CH554UART1SendByte(IO_CH552);
    CH554UART1SendByte(1);
    CH554UART1SendByte(c);
#endif
#else
    (void)c;
#endif
}

#ifdef USE_NUM_U8
void printNumU8(uint8_t num)
{
#ifdef USE_DEBUG_PRINT
    int8_t ret;
    uint8_t num_str[4] = { 0 };
    ret = uint8_to_str(num_str, 4, num);
    if (!ret) {
        printStr(num_str);
    }
#else
    (void)num;
#endif
}
#endif

#ifdef USE_NUM_U32
void printNumU32(uint32_t num)
{
#ifdef USE_DEBUG_PRINT
    int8_t ret;
    uint8_t num_str[11] = { 0 };
    ret = uint32_to_str(num_str, 10, num);
    if (!ret) {
        printStr(num_str);
    }
#else
    (void)num;
#endif
}
#endif

void printNumHex(uint8_t num)
{
#ifdef USE_DEBUG_PRINT
    // High nibble
    uint8_t val = num >> 4;
    if (val <= 9) {
        val = val + '0';
    } else {
        val = (val-10) + 'A';
    }
    printChar(val);

    // Low nibble
    val = num & 0x0F;
    if (val <= 9) {
        val = val + '0';
    } else {
        val = (val-10) + 'A';
    }
    printChar(val);
#else
    (void)num;
#endif
}

#ifdef USE_NUM_U8HEX
void printNumU8Hex(uint8_t num)
{
#ifdef USE_DEBUG_PRINT
    printStr("0x");
    printNumHex(num);
#else
    (void)num;
#endif
}
#endif

#ifdef USE_NUM_U16HEX
void printNumU16Hex(uint16_t num)
{
#ifdef USE_DEBUG_PRINT
    uint8_t buf[2];
    memcpy(buf, &num, 2);
    printStr("0x");
    for (int8_t i = 1; i > -1; i--) {
        printNumHex(buf[i]);
    }
#else
    (void)num;
#endif
}
#endif

#ifdef USE_NUM_U32HEX
void printNumU32Hex(uint32_t num)
{
#ifdef USE_DEBUG_PRINT
    uint8_t buf[4];
    memcpy(buf, &num, 4);
    printStr("0x");
    for (int8_t i = 3; i > -1; i--) {
        printNumHex(buf[i]);
    }
#else
    (void)num;
#endif
}
#endif
