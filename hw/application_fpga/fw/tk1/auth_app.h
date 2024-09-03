// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef AUTH_APP_H
#define AUTH_APP_H

#include "partition_table.h"

#include <stdbool.h>

void auth_app_create(auth_metadata_t *auth_table);
bool auth_app_authenticate(auth_metadata_t *auth_table);

#endif
