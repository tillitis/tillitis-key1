// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "flash.h"
#include "mgmt_app.h"
#include "partition_table.h"
#include "preload_app.h"

static uint32_t slot_to_start_address(uint8_t slot)
{
	return ADDR_PRE_LOADED_APP_0 + slot * SIZE_PRE_LOADED_APP;
}

/* Loads a preloaded app from flash to app RAM */
int preload_load(struct partition_table *part_table, uint8_t from_slot)
{
	if (part_table == NULL) {
		return -5;
	}

	if (from_slot >= N_PRELOADED_APP) {
		return -4;
	}

	/*Check for a valid app in flash	*/
	if (part_table->pre_app_data[from_slot].size == 0) {
		return -1;
	}
	uint8_t *loadaddr = (uint8_t *)TK1_RAM_BASE;

	/* Read from flash, straight into RAM */
	int ret = flash_read_data(slot_to_start_address(from_slot), loadaddr,
				  part_table->pre_app_data[from_slot].size);

	return ret;
}

/* Expects to receive chunks of data up to 4096 bytes to store into the
 * preloaded area. The offset needs to be kept and updated between each call.
 * Once done, call preload_store_finalize() with the last parameters.
 * */
int preload_store(struct partition_table *part_table, uint32_t offset,
		  uint8_t *data, size_t size, uint8_t to_slot)
{
	if (part_table == NULL || data == NULL) {
		return -5;
	}

	if (to_slot >= N_PRELOADED_APP) {
		return -4;
	}

	/* Check if we are allowed to store */
	if (!mgmt_app_authenticate()) {
		return -3;
	}

	/* Check for a valid app in flash, bale out if it already exists */
	if (part_table->pre_app_data[to_slot].size != 0) {
		return -1;
	}

	if ((offset + size) > SIZE_PRE_LOADED_APP || size > 4096) {
		/* Writing outside of area */
		return -2;
	}

	uint32_t address = slot_to_start_address(to_slot) + offset;

	debug_puts("preload_store: write to addr: ");
	debug_putinthex(address);
	debug_lf();

	return flash_write_data(address, data, size);
}

int preload_store_finalize(struct partition_table_storage *part_table_storage,
			   size_t app_size, uint8_t app_digest[32],
			   uint8_t app_signature[64], uint8_t to_slot)
{
	struct partition_table *part_table = &part_table_storage->table;

	if (part_table == NULL || app_digest == NULL || app_signature == NULL) {
		return -5;
	}

	if (to_slot >= N_PRELOADED_APP) {
		return -4;
	}

	/* Check if we are allowed to store */
	if (!mgmt_app_authenticate()) {
		return -3;
	}

	/* Check for a valid app in flash, bale out if it already exists */
	if (part_table->pre_app_data[to_slot].size != 0) {
		return -1;
	}

	if (app_size == 0 || app_size > SIZE_PRE_LOADED_APP) {
		return -2;
	}

	part_table->pre_app_data[to_slot].size = app_size;
	memcpy_s(part_table->pre_app_data[to_slot].digest,
		 sizeof(part_table->pre_app_data[to_slot].digest), app_digest,
		 32);
	memcpy_s(part_table->pre_app_data[to_slot].signature,
		 sizeof(part_table->pre_app_data[to_slot].signature),
		 app_signature, 64);
	debug_puts("preload_*_final: size: ");
	debug_putinthex(app_size);
	debug_lf();

	if (part_table_write(part_table_storage) != 0) {
		return -6;
	}

	return 0;
}

int preload_delete(struct partition_table_storage *part_table_storage,
		   uint8_t slot)
{
	struct partition_table *part_table = &part_table_storage->table;

	if (part_table_storage == NULL) {
		return -5;
	}

	if (slot >= N_PRELOADED_APP) {
		return -4;
	}

	/* Check if we are allowed to deleted */
	if (!mgmt_app_authenticate()) {
		return -3;
	}

	/*Check for a valid app in flash	*/
	if (part_table->pre_app_data[slot].size == 0) {
		// Nothing to do.
		return 0;
	}

	part_table->pre_app_data[slot].size = 0;

	(void)memset(part_table->pre_app_data[slot].digest, 0,
		     sizeof(part_table->pre_app_data[slot].digest));

	(void)memset(part_table->pre_app_data[slot].signature, 0,
		     sizeof(part_table->pre_app_data[slot].signature));

	if (part_table_write(part_table_storage) != 0) {
		return -6;
	}

	/* Assumes the area is 64 KiB block aligned */
	flash_block_64_erase(
	    slot_to_start_address(slot)); // Erase first 64 KB block
	flash_block_64_erase(slot_to_start_address(slot) +
			     0x10000); // Erase first 64 KB block

	return 0;
}

int preload_get_digsig(struct partition_table *part_table,
		       uint8_t app_digest[32], uint8_t app_signature[64],
		       uint8_t slot)
{
	if (part_table == NULL || app_digest == NULL || app_signature == NULL) {
		return -5;
	}

	if (slot >= N_PRELOADED_APP) {
		return -4;
	}

	/* Check if we are allowed to read */
	if (!mgmt_app_authenticate()) {
		return -3;
	}

	memcpy_s(app_digest, 32, part_table->pre_app_data[slot].digest,
		 sizeof(part_table->pre_app_data[slot].digest));
	memcpy_s(app_signature, 64, part_table->pre_app_data[slot].signature,
		 sizeof(part_table->pre_app_data[slot].signature));

	return 0;
}
