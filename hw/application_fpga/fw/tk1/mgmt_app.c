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
static const uint8_t allowed_app_digest[32] = {
	0x98, 0xeb, 0x74, 0xdf, 0xd1, 0x7d, 0x06, 0xb1, 0x3f, 0xe4, 0x7d, 0x32,
	0x60, 0x71, 0xeb, 0x34, 0x0e, 0x40, 0xb1, 0x06, 0xf4, 0x0b, 0x15, 0x06,
	0x3c, 0xc5, 0xbc, 0x6d, 0x71, 0x9e, 0x4f, 0xe9,
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
