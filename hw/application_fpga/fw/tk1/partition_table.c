// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "partition_table.h"
#include "flash.h"
#include "lib.h"
#include "proto.h"

#include <stdint.h>

int part_table_init(partition_table_t *part_table)
{
	// Read from flash, if it exists, otherwise create a new one.

	flash_release_powerdown();
	memset(part_table, 0x00, sizeof(*part_table));

	flash_read_data(ADDR_PARTITION_TABLE, (uint8_t *)part_table,
			sizeof(*part_table));

	if (part_table->version != PART_TABLE_VER) {
		// Partition table is not ours. Make a new one, and store it.
		memset(part_table, 0x00, sizeof(*part_table));

		part_table->version = PART_TABLE_VER;

		for (int i = 0; i < 4; i++) {
			part_table->app_storage[i].addr_start =
			    (ADDR_STORAGE_AREA + i * SIZE_STORAGE_AREA);
			part_table->app_storage[i].size = SIZE_STORAGE_AREA;
		}

		/* Hardcode that a preloaded app exists in flash */
		part_table->pre_app_data.size = 28024;
		part_table->pre_app_data.status = 0x02;

		part_table_write(part_table);
	}

	// Now the partition table is synced between flash and RAM.

	return 0;
}

int part_table_write(partition_table_t *part_table)
{
	flash_sector_erase(ADDR_PARTITION_TABLE);
	flash_write_data(ADDR_PARTITION_TABLE, (uint8_t *)part_table,
			 sizeof(*part_table));

	return 0;
}
