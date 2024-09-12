// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef STORAGE_H
#define STORAGE_H

#include "partition_table.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int storage_allocate_area(partition_table_t *part_table);
int storage_deallocate_area(partition_table_t *part_table);
int storage_write_data(partition_table_t *part_table, uint32_t offset,
		       uint8_t *data, size_t size);
int storage_read_data(partition_table_t *part_table, uint32_t offset,
		      uint8_t *data, size_t size);

#endif
