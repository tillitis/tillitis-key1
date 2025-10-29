// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#include "spi.h"
#include <tkey/assert.h>
#include <tkey/tk1_mem.h>

#include <stddef.h>
#include <stdint.h>

// clang-format off
static volatile uint32_t *spi_en =         (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x200);
static volatile uint32_t *spi_xfer =       (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x204);
static volatile uint32_t *spi_data =       (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x208);
// clang-format on

static int spi_ready(void);
static void spi_enable(void);
static void spi_disable(void);
static void spi_write(uint8_t *cmd, size_t size);
static void spi_read(uint8_t *buf, size_t size);

// Returns non-zero when the SPI-master is ready, and zero if not
// ready. This can be used to check if the SPI-master is available
// in the hardware.
static int spi_ready(void)
{
	return *spi_xfer;
}

static void spi_enable(void)
{
	*spi_en = 1;
}

static void spi_disable(void)
{
	*spi_en = 0;
}

static void spi_write(uint8_t *cmd, size_t size)
{
	assert(cmd != NULL);

	for (size_t i = 0; i < size; i++) {
		while (!spi_ready()) {
		}

		*spi_data = cmd[i];
		*spi_xfer = 1;
	}

	while (!spi_ready()) {
	}
}

static void spi_read(uint8_t *buf, size_t size)
{
	assert(buf != NULL);

	while (!spi_ready()) {
	}

	for (size_t i = 0; i < size; i++) {

		*spi_data = 0x00;
		*spi_xfer = 1;

		// wait until spi master is done
		while (!spi_ready()) {
		}

		buf[i] = (*spi_data & 0xff);
	}
}

// Function to both read and write data to the connected SPI flash.
int spi_transfer(uint8_t *cmd, size_t cmd_size, uint8_t *tx_buf, size_t tx_size,
		 uint8_t *rx_buf, size_t rx_size)
{
	if (cmd == NULL || cmd_size == 0) {
		return -1;
	}

	spi_enable();

	spi_write(cmd, cmd_size);

	if (tx_buf != NULL && tx_size != 0) {
		spi_write(tx_buf, tx_size);
	}

	if (rx_buf != NULL && rx_size != 0) {
		spi_read(rx_buf, rx_size);
	}

	spi_disable();

	return 0;
}
