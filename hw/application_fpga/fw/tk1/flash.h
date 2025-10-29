// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_FLASH_H
#define TKEY_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WRITE_ENABLE 0x06
#define WRITE_DISABLE 0x04

#define READ_STATUS_REG_1 0x05
#define READ_STATUS_REG_2 0x35
#define WRITE_STATUS_REG 0x01

#define PAGE_PROGRAM 0x02
#define SECTOR_ERASE 0x20
#define BLOCK_ERASE_32K 0x52
#define BLOCK_ERASE_64K 0xD8
#define CHIP_ERASE 0xC7

#define POWER_DOWN 0xB9
#define READ_DATA 0x03
#define RELEASE_POWER_DOWN 0xAB

#define READ_MANUFACTURER_ID 0x90
#define READ_JEDEC_ID 0x9F
#define READ_UNIQUE_ID 0x4B

#define ENABLE_RESET 0x66
#define RESET 0x99

#define ADDR_BYTE_3_BIT 16
#define ADDR_BYTE_2_BIT 8
#define ADDR_BYTE_1_BIT 0

#define STATUS_REG_BUSY_BIT 0
#define STATUS_REG_WEL_BIT 1

void flash_write_disable(void);
void flash_sector_erase(uint32_t address);
void flash_block_32_erase(uint32_t address);
void flash_block_64_erase(uint32_t address);
void flash_release_powerdown(void);
void flash_powerdown(void);
void flash_read_manufacturer_device_id(uint8_t *device_id);
void flash_read_jedec_id(uint8_t *jedec_id);
void flash_read_unique_id(uint8_t *unique_id);
void flash_read_status(uint8_t *status_reg);
int flash_read_data(uint32_t address, uint8_t *dest_buf, size_t size);
int flash_write_data(uint32_t address, uint8_t *data, size_t size);

#endif
