// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "spi.h"
#include "../tk1_mem.h"

#include <stddef.h>
#include <stdint.h>

// clang-format off
static volatile uint32_t *spi_en =         (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x200);
static volatile uint32_t *spi_xfer =       (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x204);
static volatile uint32_t *spi_data =       (volatile uint32_t *)(TK1_MMIO_TK1_BASE | 0x208);
// clang-format on

// returns non-zero when the SPI-master is ready, and zero if not
// ready. This can be used to check if the SPI-master is available
// in the hardware.
int spi_ready(void)
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

static void _spi_write(uint8_t *cmd, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		while (!spi_ready()) {
		}

		*spi_data = cmd[i];
		*spi_xfer = 1;
	}

	while (!spi_ready()) {
	}
}

static void _spi_read(uint8_t *buf, size_t size)
{

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

int spi_write(uint8_t *cmd, size_t cmd_size, uint8_t *data, size_t data_size)
{
	if (cmd == NULL || cmd_size == 0) {
		return -1;
	}

	spi_enable();

	_spi_write(cmd, cmd_size);

	if (data != NULL && data_size != 0) {
		_spi_write(data, data_size);
	}

	spi_disable();

	return 0;
}

int spi_transfer(uint8_t *tx_buf, size_t tx_size, uint8_t *rx_buf,
		 size_t rx_size)
{
	if (tx_buf == NULL || tx_size == 0) {
		return -1;
	}

	spi_enable();

	_spi_write(tx_buf, tx_size);

	if (rx_buf != NULL && rx_size != 0) {
		_spi_read(rx_buf, rx_size);
	}

	spi_disable();

	return 0;
}
