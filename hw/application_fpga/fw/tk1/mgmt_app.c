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
    0xc0, 0xa2, 0x8e, 0x4e, 0x35, 0x90, 0xe3, 0x4,  0x8,  0x63, 0xd4,
    0x2e, 0x51, 0xb,  0x6f, 0xc7, 0x2,	0x8b, 0xf6, 0x33, 0x18, 0x9f,
    0xca, 0xcc, 0xe8, 0x33, 0xf0, 0xd5, 0xd2, 0x72, 0x68, 0xd7,
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
