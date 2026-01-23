// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "auth_app.h"
#include "flash.h"
#include "partition_table.h"
#include "storage.h"

// Returns the index of the first empty area.
//
// Returns -1 on errors.
static int get_first_empty(struct partition_table *part_table)
{
	if (part_table == NULL) {
		return -1;
	}

	for (uint8_t i = 0; i < N_STORAGE_AREA; i++) {
		if (part_table->app_storage[i].status == 0x00) {
			return i;
		}
	}

	return -1;
}

static int index_to_address(int index, uint32_t *address)
{
	if (address == NULL) {
		return -1;
	}

	if ((index < 0) || (index >= N_STORAGE_AREA)) {
		return -1;
	}

	*address = ADDR_STORAGE_AREA + index * SIZE_STORAGE_AREA;

	return 0;
}

// Returns the index of the area an app has allocated.
//
// Returns -1 on errors.
static int storage_get_area(struct partition_table *part_table)
{
	if (part_table == NULL) {
		return -1;
	}

	for (uint8_t i = 0; i < N_STORAGE_AREA; i++) {
		if (part_table->app_storage[i].status != 0x00) {
			if (auth_app_authenticate(
				&part_table->app_storage[i].auth)) {
				return i;
			}
		}
	}

	return -1;
}

// Allocate a new area for an app. Returns zero on success.
int storage_allocate_area(struct partition_table_storage *part_table_storage)
{
	if (part_table_storage == NULL) {
		return -1;
	}

	struct partition_table *part_table = &part_table_storage->table;

	if (storage_get_area(part_table) != -1) {
		/* Already has an area */
		return 0;
	}
#ifdef DEV_ONLY_USE_AREA_0
	int index = 0;
#else
	int index = get_first_empty(part_table);
#endif

	if (index < 0) {
		/* No empty slot */
		return -1;
	}

	uint32_t start_address = 0;

	if (index_to_address(index, &start_address) != 0) {
		return -1;
	}

	// Allocate the empty index found
	// Erase area first

#ifndef DEV_ONLY_USE_AREA_0
	// Assumes the area is 64 KiB block aligned
	flash_block_64_erase(start_address); // Erase first 64 KB block
	flash_block_64_erase(start_address +
			     0x10000); // Erase second 64 KB block
#endif

	// Write partition table lastly
	part_table->app_storage[index].status = 0x01;
	auth_app_create(&part_table->app_storage[index].auth);

	if (part_table_write(part_table_storage) != 0) {
		return -1;
	}

	return 0;
}

// Dealloacate a previously allocated storage area.
//
// Returns zero on success, and non-zero on errors.
int storage_deallocate_area(struct partition_table_storage *part_table_storage)
{
	if (part_table_storage == NULL) {
		return -1;
	}

	struct partition_table *part_table = &part_table_storage->table;

	int index = storage_get_area(part_table);
	if (index < 0) {
		// No area to deallocate
		return -1;
	}

	uint32_t start_address = 0;
	if (index_to_address(index, &start_address) != 0) {
		return -1;
	}

	// Erase area first

	// Assumes the area is 64 KiB block aligned
	flash_block_64_erase(start_address); // Erase first 64 KB block
	flash_block_64_erase(start_address +
			     0x10000); // Erase second 64 KB block

	// Clear partition table lastly
	part_table->app_storage[index].status = 0;

	(void)memset(part_table->app_storage[index].auth.nonce, 0x00,
		     sizeof(part_table->app_storage[index].auth.nonce));

	(void)memset(
	    part_table->app_storage[index].auth.authentication_digest, 0x00,
	    sizeof(part_table->app_storage[index].auth.authentication_digest));

	if (part_table_write(part_table_storage) != 0) {
		return -1;
	}

	return 0;
}

// Erases sector. Offset of a sector to begin erasing, must be a
// multiple of the sector size. Size to erase in bytes, must be a
// multiple of the sector size.
//
// Returns zero on success, negative error code on failure
int storage_erase_sector(struct partition_table *part_table, uint32_t offset,
			 size_t size)
{
	if (part_table == NULL) {
		return -1;
	}

	int index = storage_get_area(part_table);
	if (index == -1) {
		// No allocated area
		return -1;
	}

	uint32_t start_address = 0;
	if (index_to_address(index, &start_address) != 0) {
		return -1;
	}

	if (offset > SIZE_STORAGE_AREA) {
		return -1;
	}

	// Cannot only erase entire sectors
	if (offset % 4096 != 0) {
		return -1;
	}

	// Cannot erase less than one sector
	if (size < 4096 || size > SIZE_STORAGE_AREA || size % 4096 != 0) {
		return -1;
	}

	if ((offset + size) > SIZE_STORAGE_AREA) {
		return -1;
	}

	uint32_t address = start_address + offset;

	debug_puts("storage: erase addr: ");
	debug_putinthex(address);
	debug_lf();

	for (size_t i = 0; i < size; i += 4096) {
		flash_sector_erase(address);
		address += 4096;
	}

	return 0;
}

// Writes the specified data to the offset inside of the allocated area.
// Assumes area has been erased before hand. Offset must be a multiple of 256.
//
// Returns zero on success.
int storage_write_data(struct partition_table *part_table, uint32_t offset,
		       uint8_t *data, size_t size)
{
	if (part_table == NULL) {
		return -1;
	}

	// Allow data to point only to app RAM
	if (data < (uint8_t *)TK1_RAM_BASE ||
	    data >= (uint8_t *)(TK1_RAM_BASE + TK1_RAM_SIZE)) {
		return -1;
	}

	int index = storage_get_area(part_table);
	if (index == -1) {
		// No allocated area
		return -1;
	}

	uint32_t start_address = 0;
	if (index_to_address(index, &start_address) != 0) {
		return -1;
	}

	if (offset > SIZE_STORAGE_AREA) {
		return -1;
	}

	if (size > SIZE_STORAGE_AREA) {
		return -1;
	}

	if ((offset + size) > SIZE_STORAGE_AREA) {
		// Writing outside of area
		return -1;
	}

	uint32_t address = start_address + offset;

	debug_puts("storage: write to addr: ");
	debug_putinthex(address);
	debug_lf();

	return flash_write_data(address, data, size);
}

// Reads size bytes of data at the specified offset inside of the
// allocated area.
//
// Only read limit is the size of the allocated area.
//
// Returns zero on success.
int storage_read_data(struct partition_table *part_table, uint32_t offset,
		      uint8_t *data, size_t size)
{
	if (part_table == NULL) {
		return -1;
	}

	// Allow data to point only to app RAM
	if (data < (uint8_t *)TK1_RAM_BASE ||
	    data >= (uint8_t *)(TK1_RAM_BASE + TK1_RAM_SIZE)) {
		return -1;
	}

	int index = storage_get_area(part_table);
	if (index == -1) {
		// No allocated area
		return -1;
	}

	uint32_t start_address = 0;

	if (index_to_address(index, &start_address) != 0) {
		return -1;
	}

	if (offset > SIZE_STORAGE_AREA) {
		return -1;
	}

	if (size > 4096) {
		return -1;
	}

	if ((offset + size) > SIZE_STORAGE_AREA) {
		// Reading outside of area
		return -1;
	}

	uint32_t address = start_address + offset;

	debug_puts("storage: read from addr: ");
	debug_putinthex(address);
	debug_lf();

	return flash_read_data(address, data, size);
}
