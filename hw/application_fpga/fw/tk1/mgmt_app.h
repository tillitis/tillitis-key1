// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MGMT_APP_H
#define MGMT_APP_H

#include "partition_table.h"

#include <stdbool.h>

bool mgmt_app_authenticate(struct management_app_metadata *mgmt_table);
int mgmt_app_register(struct partition_table *part_table);
int mgmt_app_unregister(struct partition_table *part_table);

#endif
