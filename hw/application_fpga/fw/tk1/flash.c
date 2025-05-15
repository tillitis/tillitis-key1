// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/tk1_mem.h>

#include "flash.h"
#include "spi.h"

#define PAGE_SIZE 256

static bool flash_is_busy(void);
static void flash_wait_busy(void);
static void flash_write_enable(void);

static bool flash_is_busy(void)
{
	uint8_t tx_buf = READ_STATUS_REG_1;
	uint8_t rx_buf = {0x00};

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, &rx_buf,
			    sizeof(rx_buf)) == 0);

	if (rx_buf & (1 << STATUS_REG_BUSY_BIT)) {
		return true;
	}

	return false;
}

// Blocking until !busy
static void flash_wait_busy(void)
{
	while (flash_is_busy())
		;
}

static void flash_write_enable(void)
{
	uint8_t tx_buf = WRITE_ENABLE;

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
}

void flash_write_disable(void)
{
	uint8_t tx_buf = WRITE_DISABLE;

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
}

void flash_sector_erase(uint32_t address)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = SECTOR_ERASE;
	tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
	tx_buf[2] = (address >> ADDR_BYTE_2_BIT) & 0xFF;
	/* tx_buf[3] is within a sector, and hence does not make a difference */

	flash_write_enable();
	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
	flash_wait_busy();
}

void flash_block_32_erase(uint32_t address)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = BLOCK_ERASE_32K;
	tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
	tx_buf[2] = (address >> ADDR_BYTE_2_BIT) & 0xFF;
	tx_buf[3] = (address >> ADDR_BYTE_1_BIT) & 0xFF;

	flash_write_enable();
	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
	flash_wait_busy();
}

// 64 KiB block erase, only cares about address bits 16 and above.
void flash_block_64_erase(uint32_t address)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = BLOCK_ERASE_64K;
	tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
	/* tx_buf[2] and tx_buf[3] is within a block, and hence does not make a
	 * difference */

	flash_write_enable();
	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
	flash_wait_busy();
}

void flash_release_powerdown(void)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = RELEASE_POWER_DOWN;

	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
}

void flash_powerdown(void)
{
	uint8_t tx_buf = POWER_DOWN;

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0) == 0);
}

void flash_read_manufacturer_device_id(uint8_t *device_id)
{
	assert(device_id != NULL);

	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = READ_MANUFACTURER_ID;

	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, device_id, 2) ==
	       0);
}

void flash_read_jedec_id(uint8_t *jedec_id)
{
	assert(jedec_id != NULL);

	uint8_t tx_buf = READ_JEDEC_ID;

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, jedec_id, 3) ==
	       0);
}

void flash_read_unique_id(uint8_t *unique_id)
{
	assert(unique_id != NULL);

	uint8_t tx_buf[5] = {0x00};
	tx_buf[0] = READ_UNIQUE_ID;

	assert(spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, unique_id, 8) ==
	       0);
}

void flash_read_status(uint8_t *status_reg)
{
	assert(status_reg != NULL);

	uint8_t tx_buf = READ_STATUS_REG_1;

	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, status_reg, 1) ==
	       0);

	tx_buf = READ_STATUS_REG_2;
	assert(spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, status_reg + 1,
			    1) == 0);
}

int flash_read_data(uint32_t address, uint8_t *dest_buf, size_t size)
{
	if (dest_buf == NULL) {
		return -1;
	}

	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = READ_DATA;
	tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
	tx_buf[2] = (address >> ADDR_BYTE_2_BIT) & 0xFF;
	tx_buf[3] = (address >> ADDR_BYTE_1_BIT) & 0xFF;

	return spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, dest_buf, size);
}

// Only handles writes where the least significant byte of the start address is
// zero.
int flash_write_data(uint32_t address, uint8_t *data, size_t size)
{
	if (data == NULL) {
		return -1;
	}

	if (size <= 0) {
		return -1;
	}

	if (address % 256 != 0) {
		return -1;
	}

	size_t left = size;
	uint8_t *p_data = data;
	size_t n_bytes = 0;

	// Page Program allows 1-256 bytes of a page to be written. A page is
	// 256 bytes. Behavior when writing past the end of a page is device
	// specific.
	//
	// We set the address LSByte to 0 and only write 256 bytes or less in
	// each transfer.
	uint8_t tx_buf[4] = {
	    PAGE_PROGRAM,			 /* tx_buf[0] */
	    (address >> ADDR_BYTE_3_BIT) & 0xFF, /* tx_buf[1] */
	    (address >> ADDR_BYTE_2_BIT) & 0xFF, /* tx_buf[2] */
	    0x00,				 /* tx_buf[3] */
	};

	while (left > 0) {
		if (left >= PAGE_SIZE) {
			n_bytes = PAGE_SIZE;
		} else {
			n_bytes = left;
		}

		flash_write_enable();

		if (spi_transfer(tx_buf, sizeof(tx_buf), p_data, n_bytes, NULL,
				 0) != 0) {
			return -1;
		}

		left -= n_bytes;
		p_data += n_bytes;

		address += n_bytes;
		tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
		tx_buf[2] = (address >> ADDR_BYTE_2_BIT) & 0xFF;

		flash_wait_busy();
	}

	return 0;
}
