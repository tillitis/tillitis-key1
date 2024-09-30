// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "flash.h"
#include "../tk1_mem.h"
#include "spi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off
static volatile uint32_t *timer		        = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler	= (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status		= (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl		= (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
// clang-format on

// CPU clock frequency in Hz
#define CPUFREQ 21000000
#define PAGE_SIZE 256

static void delay(int timeout_ms)
{
	// Tick once every centisecond
	*timer_prescaler = CPUFREQ / 100;
	*timer = timeout_ms / 10;

	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_START_BIT);

	while (*timer_status != 0) {
	}

	// Stop timer
	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_STOP_BIT);
}

bool flash_is_busy(void)
{
	uint8_t tx_buf = READ_STATUS_REG_1;
	uint8_t rx_buf = {0x00};

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, &rx_buf, sizeof(rx_buf));

	if (rx_buf & (1 << STATUS_REG_BUSY_BIT)) {
		return true;
	}

	return false;
}

// Blocking until !busy
void flash_wait_busy(void)
{
	while (flash_is_busy()) {
		delay(10);
	}
}

void flash_write_enable(void)
{
	uint8_t tx_buf = WRITE_ENABLE;

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
}

void flash_write_disable(void)
{
	uint8_t tx_buf = WRITE_DISABLE;

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
}

void flash_sector_erase(uint32_t address)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = SECTOR_ERASE;
	tx_buf[1] = (address >> ADDR_BYTE_3_BIT) & 0xFF;
	tx_buf[2] = (address >> ADDR_BYTE_2_BIT) & 0xFF;
	/* tx_buf[3] is within a sector, and hence does not make a difference */

	flash_write_enable();
	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
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
	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
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
	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
	flash_wait_busy();
}

void flash_release_powerdown(void)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = RELEASE_POWER_DOWN;

	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
}

void flash_powerdown(void)
{
	uint8_t tx_buf = POWER_DOWN;

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, NULL, 0);
}

void flash_read_manufacturer_device_id(uint8_t *device_id)
{
	uint8_t tx_buf[4] = {0x00};
	tx_buf[0] = READ_MANUFACTURER_ID;

	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, device_id, 2);
}

void flash_read_jedec_id(uint8_t *jedec_id)
{
	uint8_t tx_buf = READ_JEDEC_ID;

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, jedec_id, 3);
}

void flash_read_unique_id(uint8_t *unique_id)
{
	uint8_t tx_buf[5] = {0x00};
	tx_buf[0] = READ_UNIQUE_ID;

	spi_transfer(tx_buf, sizeof(tx_buf), NULL, 0, unique_id, 8);
}

void flash_read_status(uint8_t *status_reg)
{
	uint8_t tx_buf = READ_STATUS_REG_1;

	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, status_reg, 1);

	tx_buf = READ_STATUS_REG_2;
	spi_transfer(&tx_buf, sizeof(tx_buf), NULL, 0, status_reg + 1, 1);
}

int flash_read_data(uint32_t address, uint8_t *dest_buf, size_t size)
{
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
	if (size <= 0 || size > 4096) {
		return -1;
	}

	size_t left = size;
	uint8_t *p_data = data;
	size_t n_bytes = 0;

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
