#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdint.h>

#define DEBUG_PRINT
//#define DEBUG_SETUP
//#define UART_OUT_DEBUG
//#define USE_NUM_U8
#define USE_NUM_U32
//#define USE_NEGATIVE_NUMS

void printStr(uint8_t *str);
void printChar(uint8_t c);

#ifdef DEBUG_SETUP
#define printStrSetup(x)       printStr(x)
#define printNumHexSetup(x)    printNumHex(x)
#else
#define printStrSetup(x)
#define printNumHexSetup(x)
#endif

#ifdef USE_NUM_U8
int8_t uint8_to_str(uint8_t *buf, uint8_t bufsize, uint8_t n);
#endif

#ifdef USE_NUM_U32
int8_t uint32_to_str(uint8_t *buf, uint8_t bufsize, uint32_t n);
#endif

#ifdef USE_NUM_U8
void printNumU8(uint8_t num);
#endif

#ifdef USE_NUM_U32
void printNumU32(uint32_t num);
#endif

void printNumHex(uint8_t num);

#endif
