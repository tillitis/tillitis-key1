// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STORAGE_H
#define STORAGE_H

#include "partition_table.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int storage_deallocate_area(struct partition_table_storage *part_table_storage);
int storage_allocate_area(struct partition_table_storage *part_table_storage);
int storage_erase_sector(struct partition_table *part_table, uint32_t offset,
			 size_t size);
int storage_write_data(struct partition_table *part_table, uint32_t offset,
		       uint8_t *data, size_t size);
int storage_read_data(struct partition_table *part_table, uint32_t offset,
		      uint8_t *data, size_t size);

#endif
