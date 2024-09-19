// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PRELOAD_APP_H
#define PRELOAD_APP_H

#include "partition_table.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool preload_check_valid_app(partition_table_t *part_table);
int preload_start(partition_table_t *part_table);
int preload_store(partition_table_t *part_table, uint32_t offset, uint8_t *data,
		  size_t size);
int preload_store_finalize(partition_table_t *part_table, bool use_uss,
			   uint8_t *uss, size_t app_size);
int preload_delete(partition_table_t *part_table);

#endif
