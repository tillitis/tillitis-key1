// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PRELOAD_APP_H
#define PRELOAD_APP_H

#include "partition_table.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool preload_check_valid_app(struct partition_table *part_table,
			     uint8_t slot);
int preload_load(struct partition_table *part_table, uint8_t from_slot);
int preload_store(struct partition_table *part_table, uint32_t offset,
		  uint8_t *data, size_t size, uint8_t to_slot);
int preload_store_finalize(struct partition_table *part_table, bool use_uss,
			   uint8_t *uss, size_t app_size, uint8_t to_slot);
int preload_delete(struct partition_table *part_table, uint8_t slot);

#endif
