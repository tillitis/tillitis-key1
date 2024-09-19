// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "preload_app.h"
#include "../tk1_mem.h"
#include "flash.h"
#include "htif.h"
#include "lib.h"
#include "mgmt_app.h"
#include "partition_table.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Returns non-zero if the app is valid */
bool preload_check_valid_app(partition_table_t *part_table)
{

	if (part_table->pre_app_data.status == 0x00 &&
	    part_table->pre_app_data.size == 0) {
		/*No valid app*/
		return false;
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

	/* Read from flash, straight into RAM */
	int ret = flash_read_data(ADDR_PRE_LOADED_APP, loadaddr,
				  part_table->pre_app_data.size);

	return ret;
}

/* Expects to receive chunks of data up to 4096 bytes to store into the
 * preloaded area. The offset needs to be kept and updated between each call.
 * Once done, call preload_store_finalize() with the last parameters.
 * */
int preload_store(partition_table_t *part_table, uint32_t offset, uint8_t *data,
		  size_t size)
{
	/* Check if we are allowed to store */
	if (!mgmt_app_authenticate(&part_table->mgmt_app_data)) {
		return -3;
	}

	/* Check for a valid app in flash, bale out if it already exists */
	if (preload_check_valid_app(part_table)) {
		return -1;
	}

	if ((offset + size) > SIZE_PRE_LOADED_APP || size > 4096) {
		/* Writing outside of area */
		return -2;
	}

	uint32_t address = ADDR_PRE_LOADED_APP + offset;

	htif_puts("preload_store: write to addr: ");
	htif_putinthex(address);
	htif_lf();

	return flash_write_data(address, data, size);
}

int preload_store_finalize(partition_table_t *part_table, bool use_uss,
			   uint8_t *uss, size_t app_size)
{
	/* Check if we are allowed to store */
	if (!mgmt_app_authenticate(&part_table->mgmt_app_data)) {
		return -3;
	}

	/* Check for a valid app in flash, bale out if it already exists */
	if (preload_check_valid_app(part_table)) {
		return -1;
	}

	// TODO: Maybe add the uss fields

	if (app_size == 0 || app_size > SIZE_PRE_LOADED_APP) {
		return -2;
	}

	part_table->pre_app_data.size = app_size;
	part_table->pre_app_data.status =
	    0x02; /* Stored but not yet authenticated */
	htif_puts("preload_*_final: size: ");
	htif_putinthex(app_size);
	htif_lf();

	part_table_write(part_table);

	/* Force a restart to authenticate the stored app */
	/* TODO: Should this be done by the management app or by firmware? */

	return 0;
}

int preload_delete(partition_table_t *part_table)
{
	/* Check if we are allowed to deleted */
	if (!mgmt_app_authenticate(&part_table->mgmt_app_data)) {
		return -3;
	}

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

	/* Assumes the area is 64 KiB block aligned */
	flash_block_64_erase(ADDR_PRE_LOADED_APP); // Erase first 64 KB block
	flash_block_64_erase(ADDR_PRE_LOADED_APP +
			     0x10000); // Erase second 64 KB block

	return 0;
}
