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
    0xf8, 0x90, 0x34, 0x31, 0xe0, 0xed, 0xab, 0x8b, 0x91, 0xe5, 0x63,
    0xe6, 0xea, 0x6a, 0x49, 0xe6, 0x53, 0x1e, 0xc7, 0x47, 0xca, 0x2f,
    0x2b, 0x8f, 0xf3, 0x5e, 0x67, 0x13, 0x0f, 0xfa, 0x93, 0x36,
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

// Authenticate an management app
bool mgmt_app_authenticate(void)
{
	return memeq(current_app_digest, allowed_app_digest, 32) != 0;
}

uint8_t *mgmt_app_allowed_digest(void)
{
	return (uint8_t *)allowed_app_digest;
}
