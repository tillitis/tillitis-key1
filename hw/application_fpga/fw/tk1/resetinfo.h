// Copyright (C) 2025 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TKEY_RESETINFO_H
#define TKEY_RESETINFO_H

#include <stdint.h>

enum reset_type_num {
	UNKNOWN = 0,
	LOAD_APP_FROM_HOST = 1,
	LOAD_APP_FROM_FLASH = 2,
	LOAD_APP_FROM_HOST_WITH_DIGEST = 3,
	LOAD_APP_FROM_FLASH_WITH_DIGEST = 4,
};

struct reset {
	uint32_t type;           // Reset type
	uint8_t app_digest[32];  // Program digest
};

#endif
