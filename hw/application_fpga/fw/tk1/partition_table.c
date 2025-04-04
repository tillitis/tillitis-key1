// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/lib.h>

#include "blake2s/blake2s.h"
#include "flash.h"
#include "partition_table.h"
#include "proto.h"

void part_digest(struct partition_table *part_table, uint8_t *out_digest, size_t out_len) {
	blake2s_ctx b2s_ctx = {0};
	int blake2err = 0;

	uint8_t key[16] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
	};
	blake2err = blake2s(out_digest, out_len,
	key, sizeof(key), part_table, sizeof(struct partition_table), &b2s_ctx);

	assert(blake2err == 0);
}

int part_table_read(struct partition_table_storage *storage)
{
	flash_release_powerdown();
	memset(storage, 0x00, sizeof(*storage));

	flash_read_data(ADDR_PARTITION_TABLE, (uint8_t *)storage,
			sizeof(*storage));

	// TODO: Implement redundancy

	uint8_t check_digest[PART_DIGEST_SIZE];
	part_digest(&storage->table, check_digest, sizeof(check_digest));

	if (!memeq(check_digest, storage->check_digest, sizeof(check_digest))) {
		return -1;
	}

	return 0;
}

int part_table_write(struct partition_table_storage *storage)
{
	part_digest(&storage->table, storage->check_digest, sizeof(storage->check_digest));
	flash_sector_erase(ADDR_PARTITION_TABLE);
	flash_write_data(ADDR_PARTITION_TABLE, (uint8_t *)storage,
			 sizeof(*storage));

	return 0;
}
