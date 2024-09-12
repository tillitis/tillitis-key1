// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "mgmt_app.h"
#include "auth_app.h"
#include "lib.h"
#include "partition_table.h"

#include <stdbool.h>

/* Returns true if an management app is already registered */
static bool mgmt_app_registered(management_app_metadata_t *mgmt_table)
{

	if (mgmt_table->status == 0x00) {
		/* No management app registered */
		return false;
		// TODO: Should we also check nonce, authentication digest for
		// non-zero?
	}

	return true;
}

/* Authenticate an management app */
bool mgmt_app_authenticate(management_app_metadata_t *mgmt_table)
{
	if (!mgmt_app_registered(mgmt_table)) {
		return false;
	}

	return auth_app_authenticate(&mgmt_table->auth);
}

/* Register an management app, returns zero on success */
int mgmt_app_register(partition_table_t *part_table)
{
	/* Check if the current app is the mgmt app */
	if (mgmt_app_authenticate(&part_table->mgmt_app_data)) {
		return 0;
	}

	/* Check if another management app is registered */
	if (mgmt_app_registered(&part_table->mgmt_app_data)) {
		return -1;
	}

	auth_app_create(&part_table->mgmt_app_data.auth);
	part_table->mgmt_app_data.status = 0x01;

	part_table_write(part_table);

	return 0;
}

/* Unregister the currently registered app, returns zero on success */
int mgmt_app_unregister(partition_table_t *part_table)
{
	/* Only the management app should be able to unregister itself */
	if (!mgmt_app_authenticate(&part_table->mgmt_app_data)) {
		return -1;
	}

	part_table->mgmt_app_data.status = 0;

	memset(part_table->mgmt_app_data.auth.nonce, 0x00,
	       sizeof(part_table->mgmt_app_data.auth.nonce));

	memset(part_table->mgmt_app_data.auth.authentication_digest, 0x00,
	       sizeof(part_table->mgmt_app_data.auth.authentication_digest));

	part_table_write(part_table);

	return 0;
}
