// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PRELOAD_APP_H
#define PRELOAD_APP_H

#include "partition_table.h"
#include <stdbool.h>
#include <stdint.h>

bool preload_check_valid_app(partition_table_t *part_table);
int preload_start(partition_table_t *part_table);
int preload_store(partition_table_t *part_table);
int preload_delete(partition_table_t *part_table);

#endif
