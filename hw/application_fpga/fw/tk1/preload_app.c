// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "preload_app.h"
#include "../tk1_mem.h"
#include "flash.h"
#include "lib.h"
#include "partition_table.h"

#include <stdbool.h>
#include <stdint.h>

/* Returns non-zero if the app is valid */
bool preload_check_valid_app(partition_table_t *part_table)
{

	if (part_table->pre_app_data.status == 0x00 &&
	    part_table->pre_app_data.size == 0) {
		/*No valid app*/
		return false;
		// TODO: Should we also check nonce, authentication digest for
		// non-zero?
	}

	return true;
}

/* Loads a preloaded app from flash to app RAM */
int preload_start(partition_table_t *part_table)
{
	/*Check for a valid app in flash	*/
	if (!preload_check_valid_app(part_table)) {
		return -1;
	}
	uint8_t *loadaddr = (uint8_t *)TK1_RAM_BASE;

	// TODO: Check authentication digest
	// TODO: Should this function set *app_size?

	/* Read from flash, straight into RAM */
	int ret = flash_read_data(ADDR_PRE_LOADED_APP, loadaddr,
				  part_table->pre_app_data.size);

	return ret;
}

int preload_store(partition_table_t *part_table)
{
	// TODO: Can reuse the app loading context  in main, to keep track of
	// where to store.
	//  Most likely needs to aggregate some data, before it writes to flash.

	/*Check for a valid app in flash, bale out if it already exists */
	if (preload_check_valid_app(part_table)) {
		return -1;
	}

	return 0;
}

int preload_delete(partition_table_t *part_table)
{
	/*Check for a valid app in flash	*/
	if (!preload_check_valid_app(part_table)) {
		return 0;
		// TODO: Nothing here, return zero like all is good?
	}
	part_table->pre_app_data.size = 0;
	part_table->pre_app_data.status = 0;

	memset(part_table->pre_app_data.auth.nonce, 0x00,
	       sizeof(part_table->pre_app_data.auth.nonce));

	memset(part_table->pre_app_data.auth.authentication_digest, 0x00,
	       sizeof(part_table->pre_app_data.auth.authentication_digest));

	part_table_write(part_table);

	flash_block_64_erase(ADDR_PRE_LOADED_APP); // Erase first 64 KB block
	flash_block_64_erase(ADDR_PRE_LOADED_APP +
			     0x10000); // Erase second 64 KB block

	return 0;
}
