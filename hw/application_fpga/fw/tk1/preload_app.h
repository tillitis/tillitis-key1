// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PRELOAD_APP_H
#define PRELOAD_APP_H

#include "partition_table.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int preload_load(struct partition_table *part_table, uint8_t from_slot);
int preload_store(struct partition_table *part_table, uint32_t offset,
		  uint8_t *data, size_t size, uint8_t to_slot);
int preload_store_finalize(struct partition_table_storage *part_table_storage,
			   size_t app_size, uint8_t app_digest[32],
			   uint8_t app_signature[64], uint8_t to_slot);
int preload_delete(struct partition_table_storage *part_table_storage,
		   uint8_t slot);
int preload_get_digsig(struct partition_table *part_table,
		       uint8_t app_digest[32], uint8_t app_signature[64],
		       uint8_t slot);

#endif
