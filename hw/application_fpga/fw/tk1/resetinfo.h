// Copyright (C) 2025 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef TKEY_RESETINFO_H
#define TKEY_RESETINFO_H

#include <stdint.h>

#define TK1_MMIO_RESETINFO_BASE 0xd0000f00
#define TK1_MMIO_RESETINFO_SIZE 0x100

enum reset_start {
	START_DEFAULT = 0, // Probably cold boot
	START_FLASH0 = 1,
	START_FLASH1 = 2,
	START_FLASH0_VER = 3,
	START_FLASH1_VER = 4,
	START_CLIENT = 5,
	START_CLIENT_VER = 6,
};

struct reset {
	uint32_t type;		    // Reset type
	uint8_t app_digest[32];	    // Program digest
	uint8_t next_app_data[220]; // Data to leave around for next app
};

#endif
