// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/lib.h>

#include "blake2s/blake2s.h"
#include "flash.h"
#include "keys.h"
#include "partition_table.h"
#include "proto.h"

static enum part_status part_status;
static uint8_t mac_key[32] = {0};

uint8_t *part_table_key(void)
{
	return mac_key;
}

enum part_status part_get_status(void)
{
	return part_status;
}

// part_auth computes a mac over the partition table to detect
// flash problems
static void part_mac(struct partition_table *part_table, uint8_t *out_mac,
		     size_t out_len)
{
	int blake2err = 0;

	assert(part_table != NULL);
	assert(out_mac != NULL);

	blake2err = blake2s(out_mac, out_len, mac_key, 32, part_table,
			    sizeof(struct partition_table));

	assert(blake2err == 0);
}

// part_table_read reads and verifies the partition table storage,
// first trying slot 0, then slot 1 if slot 0 does not verify.
//
// It stores the partition table in storage.
//
// Returns negative values on errors.
int part_table_read(struct partition_table_storage *storage)
{
	if (!keys_is_init()) {
		return -1;
	}

	uint32_t offset[2] = {
	    ADDR_PARTITION_TABLE_0,
	    ADDR_PARTITION_TABLE_1,
	};
	uint8_t check_mac[PART_MAC_SIZE] = {0};

	if (storage == NULL) {
		return -1;
	}

	flash_release_powerdown();
	(void)memset(storage, 0x00, sizeof(*storage));

	for (int i = 0; i < 2; i++) {
		if (flash_read_data(offset[i], (uint8_t *)storage,
				    sizeof(*storage)) != 0) {
			return -1;
		}
		part_mac(&storage->table, check_mac, sizeof(check_mac));

		if (memeq(check_mac, storage->mac, sizeof(check_mac))) {
			if (i == 1) {
				part_status = PART_SLOT0_INVALID;
			}

			return 0;
		}
	}

	return -1;
}

int part_table_write(struct partition_table_storage *storage)
{
	uint32_t offset[2] = {
	    ADDR_PARTITION_TABLE_0,
	    ADDR_PARTITION_TABLE_1,
	};

	if (storage == NULL) {
		return -1;
	}

	part_mac(&storage->table, storage->mac, sizeof(storage->mac));

	for (int i = 0; i < 2; i++) {
		flash_sector_erase(offset[i]);
		if (flash_write_data(offset[i], (uint8_t *)storage,
				     sizeof(*storage)) != 0) {
			return -1;
		}
	}

	return 0;
}
