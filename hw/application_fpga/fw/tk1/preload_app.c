// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "flash.h"
#include "memcheck.h"
#include "mgmt_app.h"
#include "partition_table.h"
#include "preload_app.h"

static uint32_t slot_to_start_address(uint8_t slot)
{
	return ADDR_PRE_LOADED_APP_0 + slot * SIZE_PRE_LOADED_APP;
}

// Loads a preloaded app from flash to app RAM
int preload_load(struct partition_table *part_table, uint8_t from_slot)
{
	if (part_table == NULL) {
		return -1;
	}

	if (from_slot >= N_PRELOADED_APP) {
		return -1;
	}

	// Check for a valid app in flash
	if (part_table->pre_app_data[from_slot].size == 0 ||
	    part_table->pre_app_data[from_slot].size > TK1_APP_MAX_SIZE) {
		return -1;
	}
	uint8_t *loadaddr = (uint8_t *)TK1_RAM_BASE;

	// Read from flash, straight into RAM
	int ret = flash_read_data(slot_to_start_address(from_slot), loadaddr,
				  part_table->pre_app_data[from_slot].size);

	return ret;
}

// preload_store stores chunks of an app in app slot to_slot. data is a buffer
// of size size to be written at byte offset in the slot. offset needs to be
// kept and updated between each call. offset must be a multiple of 256.
//
// When all data has been written call preload_set_metadata() with the last
// parameters.
//
// Returns 0 on success.
int preload_store(struct partition_table *part_table, uint32_t offset,
		  uint8_t *data, size_t size, uint8_t to_slot)
{
	if (part_table == NULL) {
		return -1;
	}

	if (!in_app_ram(data, size)) {
		return -1;
	}

	if (to_slot >= N_PRELOADED_APP) {
		return -1;
	}

	// Check if we are allowed to store
	if (!mgmt_app_authenticate()) {
		return -1;
	}

	// Check for a valid app in flash, bale out if it already
	// exists
	if (part_table->pre_app_data[to_slot].size != 0) {
		return -1;
	}

	if (offset > SIZE_PRE_LOADED_APP) {
		return -1;
	}

	if (size > SIZE_PRE_LOADED_APP) {
		return -1;
	}

	if ((offset + size) > SIZE_PRE_LOADED_APP) {
		// Writing outside of area
		return -1;
	}

	uint32_t address = slot_to_start_address(to_slot) + offset;

	debug_puts("preload_store: write to addr: ");
	debug_putinthex(address);
	debug_lf();

	return flash_write_data(address, data, size);
}

// Used to write associated metadata to the preloaded app in slot to_slot.
// This includes:
// - Updating the public key.
// - Finalizing and storage session, i.e., storing an app with new size,
// signature and digest.
//
// Only size and slot is validated, the rest is up to the caller to validate.
//
// Privileged function.
// Returns zero on success.
int preload_set_metadata(struct partition_table_storage *part_table_storage,
			 struct app_metadata *metadata, uint8_t to_slot)
{
	struct partition_table *part_table = &part_table_storage->table;

	if (part_table == NULL) {
		return -1;
	}

	if (!in_app_ram(metadata, sizeof(struct app_metadata))) {
		return -1;
	}

	if (to_slot >= N_PRELOADED_APP) {
		return -1;
	}

	// Check if we are allowed to store
	if (!mgmt_app_authenticate()) {
		return -1;
	}

	if (metadata->size == 0 || metadata->size > SIZE_PRE_LOADED_APP) {
		return -1;
	}

	part_table->pre_app_data[to_slot].size = metadata->size;
	memcpy_s(part_table->pre_app_data[to_slot].digest,
		 sizeof(part_table->pre_app_data[to_slot].digest),
		 metadata->digest, sizeof(metadata->digest));
	memcpy_s(part_table->pre_app_data[to_slot].signature,
		 sizeof(part_table->pre_app_data[to_slot].signature),
		 metadata->signature, sizeof(metadata->signature));
	memcpy_s(part_table->pre_app_data[to_slot].pubkey,
		 sizeof(part_table->pre_app_data[to_slot].pubkey),
		 metadata->pubkey, sizeof(metadata->pubkey));
	memcpy_s(part_table->pre_app_data[to_slot].mac,
		 sizeof(part_table->pre_app_data[to_slot].mac), metadata->mac,
		 sizeof(metadata->mac));

	debug_puts("preload_set_metadata: size: ");
	debug_putinthex(metadata->size);
	debug_lf();

	if (part_table_write(part_table_storage) != 0) {
		return -1;
	}

	return 0;
}

// Delete preloaded app in slot and update partition table.
//
// Erases preloaded app from the slot.
// Sets size, digest and signature to zero.
// Validates metadata->size is zero, the rest is up to the caller to validate.
// Updates the mac from the metadata->mac. Does not update pubkey.
//
// Privileged function.
// Returns zero on success
int preload_delete(struct partition_table_storage *part_table_storage,
		   struct app_metadata *metadata, uint8_t slot)
{
	struct partition_table *part_table = &part_table_storage->table;

	if (part_table_storage == NULL) {
		return -1;
	}

	if (!in_app_ram(metadata, sizeof(struct app_metadata))) {
		return -1;
	}

	if (slot >= N_PRELOADED_APP) {
		return -1;
	}

	if (metadata->size != 0) {
		return -1;
	}

	// Check if we are allowed to delete
	if (!mgmt_app_authenticate()) {
		return -1;
	}

	// Check for a valid app in flash
	if (part_table->pre_app_data[slot].size == 0) {
		// Nothing to do.
		return 0;
	}

	part_table->pre_app_data[slot].size = 0;

	(void)memset(part_table->pre_app_data[slot].digest, 0,
		     sizeof(part_table->pre_app_data[slot].digest));

	(void)memset(part_table->pre_app_data[slot].signature, 0,
		     sizeof(part_table->pre_app_data[slot].signature));

	memcpy_s(part_table->pre_app_data[slot].mac,
		 sizeof(part_table->pre_app_data[slot].mac), metadata->mac,
		 sizeof(metadata->mac));

	if (part_table_write(part_table_storage) != 0) {
		return -1;
	}

	// Assumes the area is 64 KiB block aligned
	flash_block_64_erase(
	    slot_to_start_address(slot)); // Erase first 64 KB block
	flash_block_64_erase(slot_to_start_address(slot) +
			     0x10000); // Erase second 64 KB block

	return 0;
}

// Get metadata for the preloaded app in slot slot.
// Privileged function.
// Returns zero on success.
int preload_get_metadata(struct partition_table *part_table,
			 struct app_metadata *metadata, uint8_t slot)
{
	if (part_table == NULL) {
		return -1;
	}

	if (!in_app_ram(metadata, sizeof(struct app_metadata))) {
		return -1;
	}

	if (slot >= N_PRELOADED_APP) {
		return -1;
	}

	// Check if we are allowed to read
	if (!mgmt_app_authenticate()) {
		return -1;
	}

	metadata->size = part_table->pre_app_data[slot].size;
	memcpy_s(metadata->digest, sizeof(metadata->digest),
		 part_table->pre_app_data[slot].digest,
		 sizeof(part_table->pre_app_data[slot].digest));
	memcpy_s(metadata->signature, sizeof(metadata->signature),
		 part_table->pre_app_data[slot].signature,
		 sizeof(part_table->pre_app_data[slot].signature));
	memcpy_s(metadata->pubkey, sizeof(metadata->pubkey),
		 part_table->pre_app_data[slot].pubkey,
		 sizeof(part_table->pre_app_data[slot].pubkey));
	memcpy_s(metadata->mac, sizeof(metadata->mac),
		 part_table->pre_app_data[slot].mac,
		 sizeof(part_table->pre_app_data[slot].mac));

	return 0;
}
