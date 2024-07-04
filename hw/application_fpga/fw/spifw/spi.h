// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TKEY_SPI_H
#define TKEY_SPI_H

// #include <stddef.h>
// #include <stdint.h>
#include "../tk1/types.h"

int spi_ready(void);
int spi_write(uint8_t *cmd, size_t size_cmd, uint8_t *data, size_t size_data);
int spi_transfer(uint8_t *tx_buf, size_t tx_size, uint8_t *rx_buf,
		 size_t rx_size);

#endif
