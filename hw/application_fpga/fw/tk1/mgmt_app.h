// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MGMT_APP_H
#define MGMT_APP_H

#include "partition_table.h"

#include <stdbool.h>

bool mgmt_app_authenticate(management_app_metadata_t *mgmt_table);
int mgmt_app_register(partition_table_t *part_table);
int mgmt_app_unregister(partition_table_t *part_table);

#endif
