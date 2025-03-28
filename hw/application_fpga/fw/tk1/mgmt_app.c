// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <tkey/lib.h>
#include <stdbool.h>
#include <stdint.h>

#include "mgmt_app.h"

// Locked down what app can start from first flash slot to be exactly
// this size, producing this digest.
//
// To update this, compute the BLAKE2s digest of the app.bin
// BLAKE2s digest of testloadapp.bin
static const uint8_t allowed_app_digest[32] = {
	0x88, 0x87, 0xf6, 0x2f, 0x71, 0x1c, 0x3f, 0xdb,
	0x8c, 0xf7, 0x77, 0x9f, 0xeb, 0x5f, 0xb9, 0xd4,
	0x2f, 0xfb, 0xdb, 0x1d, 0xf6, 0xdc, 0x62, 0xff,
	0x91, 0x00, 0x1f, 0x5d, 0x98, 0xb3, 0x50, 0xd4,
};
static uint8_t current_app_digest[32];

int mgmt_app_init(uint8_t app_digest[32]) {
	if (memeq(app_digest, allowed_app_digest, 32)) {
		memcpy_s(current_app_digest, sizeof(current_app_digest), app_digest, 32);
		return 0;
	}

	return -1;
}

/* Authenticate an management app */
bool mgmt_app_authenticate(void)
{
	return memeq(current_app_digest, allowed_app_digest, 32) != 0;
}
