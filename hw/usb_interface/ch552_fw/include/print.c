#include <stdint.h>

#include "debug.h"
#include "print.h"

void printStr(uint8_t *str)
{
#ifdef DEBUG_PRINT
    while (*str != 0) {
        CH554UART0SendByte(*str);
        ++str;
    }
#else
    (void)str;
#endif
}

void printChar(uint8_t c)
{
#ifdef DEBUG_PRINT
    CH554UART0SendByte(c);
#else
    (void)c;
#endif
}

#ifdef USE_NUM_U8
int8_t uint8_to_str(uint8_t *buf, uint8_t bufsize, uint8_t n)
{
    uint8_t *start;

#ifdef USE_NEGATIVE_NUMS
    if (n < 0) { // Handle negative numbers.
        if (!bufsize) {
            return -1;
        }
        *buf++ = '-';
        bufsize--;
    }
#endif

    start = buf; // Remember the start of the string. This will come into play at the end.

    do {
        // Handle the current digit.
        uint8_t digit;
        if (!bufsize) {
            return -1;
        }
        digit = n % 10;
#ifdef USE_NEGATIVE_NUMS
        if (digit < 0) {
            digit *= -1;
        }
#endif
        *buf++ = digit + '0';
        bufsize--;
        n /= 10;
    } while (n);

    // Terminate the string.
    if (!bufsize) {
        return -1;
    }
    *buf = 0;

    // We wrote the string backwards, i.e. with least significant digits first. Now reverse the string.
    --buf;
    while (start < buf) {
        uint8_t a = *start;
        *start = *buf;
        *buf = a;
        ++start;
        --buf;
    }

    return 0;
}
#endif

#ifdef USE_NUM_U32
int8_t uint32_to_str(uint8_t *buf, uint8_t bufsize, uint32_t n)
{
    uint8_t *start;

#ifdef USE_NEGATIVE_NUMS
    if (n < 0) { // Handle negative numbers.
        if (!bufsize) {
            return -1;
        }
        *buf++ = '-';
        bufsize--;
    }
#endif

    start = buf; // Remember the start of the string. This will come into play at the end.

    do {
        // Handle the current digit.
        uint8_t digit;
        if (!bufsize) {
            return -1;
        }
        digit = n % 10;
#ifdef USE_NEGATIVE_NUMS
        if (digit < 0) {
            digit *= -1;
        }
#endif
        *buf++ = digit + '0';
        bufsize--;
        n /= 10;
    } while (n);

    // Terminate the string.
    if (!bufsize) {
        return -1;
    }
    *buf = 0;

    // We wrote the string backwards, i.e. with least significant digits first. Now reverse the string.
    --buf;
    while (start < buf) {
        uint8_t a = *start;
        *start = *buf;
        *buf = a;
        ++start;
        --buf;
    }

    return 0;
}
#endif

#ifdef USE_NUM_U8
void printNumU8(uint8_t num)
{
#ifdef DEBUG_PRINT
    uint8_t num_str[4] = { 0 };
    int8_t ret;
    ret = uint8_to_str(num_str, 4, num);
    if (!ret) {
        printStr(num_str);
    }
#endif
}
#endif

#ifdef USE_NUM_U32
void printNumU32(uint32_t num)
{
#ifdef DEBUG_PRINT
    uint8_t num_str[11] = { 0 };
    int8_t ret;
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
#ifdef DEBUG_PRINT
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
