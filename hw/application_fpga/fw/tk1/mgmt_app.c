// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <tkey/lib.h>
#include <stdbool.h>
#include <stdint.h>
#include <tkey/io.h>

#include "mgmt_app.h"

// Lock down what app can start from flash slot 0.
//
// To update this, compute the BLAKE2s digest of the app.bin
static const uint8_t allowed_app_digest[32] = {
    0x5d, 0xf0, 0x37, 0x3a, 0x2c, 0x5a, 0xa,  0x42, 0x95, 0xb5, 0x78,
    0x2e, 0x44, 0xa9, 0x4,  0x8e, 0xb3, 0x71, 0x11, 0x81, 0x48, 0x0,
    0x16, 0xf6, 0x67, 0x1e, 0x6b, 0x61, 0x73, 0xd4, 0x18, 0x49,
};

static uint8_t current_app_digest[32];

int mgmt_app_init(uint8_t app_digest[32]) {
	if (app_digest == NULL) {
		return -1;
	}

	memcpy_s(current_app_digest, sizeof(current_app_digest), app_digest, 32);

	return 0;
}

/* Authenticate an management app */
bool mgmt_app_authenticate(void)
{
	return memeq(current_app_digest, allowed_app_digest, 32) != 0;
}

uint8_t *mgmt_app_allowed_digest(void) {
	return (uint8_t *)allowed_app_digest;
}
