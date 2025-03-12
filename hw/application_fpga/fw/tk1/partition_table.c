// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/lib.h>

#include "blake2s/blake2s.h"
#include "flash.h"
#include "partition_table.h"
#include "proto.h"

static enum part_status part_status;

enum part_status part_get_status(void)
{
	return part_status;
}

static void part_digest(struct partition_table *part_table, uint8_t *out_digest,
			size_t out_len);

static void part_digest(struct partition_table *part_table, uint8_t *out_digest,
			size_t out_len)
{
	int blake2err = 0;

	uint8_t key[16] = {
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	assert(part_table != NULL);
	assert(out_digest != NULL);

	blake2err = blake2s(out_digest, out_len, key, sizeof(key), part_table,
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
	uint32_t offset[2] = {
	    ADDR_PARTITION_TABLE_0,
	    ADDR_PARTITION_TABLE_1,
	};
	uint8_t check_digest[PART_DIGEST_SIZE] = {0};

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
		part_digest(&storage->table, check_digest,
			    sizeof(check_digest));

		if (memeq(check_digest, storage->check_digest,
			  sizeof(check_digest))) {
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

	part_digest(&storage->table, storage->check_digest,
		    sizeof(storage->check_digest));

	for (int i = 0; i < 2; i++) {
		flash_sector_erase(offset[i]);
		if (flash_write_data(offset[i], (uint8_t *)storage,
				     sizeof(*storage)) != 0) {
			return -1;
		}
	}

	return 0;
}
