// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stdint.h>
#include <tkey/io.h>
#include <tkey/lib.h>

#include "mgmt_app.h"

// Lock down what app can start from flash slot 0.
//
// To update this, compute the BLAKE2s digest of the app.bin
static const uint8_t allowed_app_digest[32] = {
    0x7e, 0x6e, 0x12, 0x72, 0x79, 0xcc, 0x3c, 0x6a, 0xf2, 0x67, 0x28,
    0x7d, 0x72, 0xcf, 0x26, 0x85, 0x61, 0xb0, 0x62, 0x29, 0x2f, 0x56,
    0x98, 0x7a, 0xf0, 0xb,  0x3e, 0xce, 0x39, 0xde, 0x5e, 0xe3,
};

static uint8_t current_app_digest[32];

int mgmt_app_init(uint8_t app_digest[32])
{
	if (app_digest == NULL) {
		return -1;
	}

	memcpy_s(current_app_digest, sizeof(current_app_digest), app_digest,
		 32);

	return 0;
}

/* Authenticate an management app */
bool mgmt_app_authenticate(void)
{
	return memeq(current_app_digest, allowed_app_digest, 32) != 0;
}

uint8_t *mgmt_app_allowed_digest(void)
{
	return (uint8_t *)allowed_app_digest;
}
