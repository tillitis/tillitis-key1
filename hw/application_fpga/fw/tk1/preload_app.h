// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PRELOAD_APP_H
#define PRELOAD_APP_H

#include "partition_table.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/syscall.h>

int preload_load(struct partition_table *part_table, uint8_t from_slot);
int preload_store(struct partition_table *part_table, uint32_t offset,
		  uint8_t *data, size_t size, uint8_t to_slot);
int preload_get_metadata(struct partition_table *part_table,
			 struct app_metadata *metadata, uint8_t slot);
int preload_set_metadata(struct partition_table_storage *part_table_storage,
			 struct app_metadata *metadata, uint8_t slot);
int preload_delete(struct partition_table_storage *part_table_storage,
		   struct app_metadata *metadata, uint8_t slot);

#endif
