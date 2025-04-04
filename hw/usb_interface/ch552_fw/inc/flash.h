// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdint.h>

uint8_t write_code_flash(uint16_t address, uint16_t data);
uint8_t write_data_flash(uint8_t address, uint8_t data);

#endif
