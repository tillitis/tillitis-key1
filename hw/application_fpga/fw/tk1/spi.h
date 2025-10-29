// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_SPI_H
#define TKEY_SPI_H

#include <stddef.h>
#include <stdint.h>

int spi_transfer(uint8_t *cmd, size_t cmd_size, uint8_t *tx_buf, size_t tx_size,
		 uint8_t *rx_buf, size_t rx_size);

#endif
