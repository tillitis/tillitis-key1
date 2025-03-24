#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdint.h>

#include "config.h"

void printStr(uint8_t *str);
void printChar(uint8_t c);

#ifdef DEBUG_SETUP
#define printStrSetup(str)       printStr(str)
#define printNumU8HexSetup(num)  printNumU8Hex(num)
#else
#define printStrSetup(str)
#define printNumU8HexSetup(num)
#endif

#ifdef USE_NUM_U8
void printNumU8(uint8_t num);
#endif

#ifdef USE_NUM_U32
void printNumU32(uint32_t num);
#endif

#ifdef USE_NUM_U8HEX
void printNumU8Hex(uint8_t num);
#endif

#ifdef USE_NUM_U16HEX
void printNumU16Hex(uint16_t num);
#endif

#ifdef USE_NUM_U32HEX
void printNumU32Hex(uint32_t num);
#endif

#endif
