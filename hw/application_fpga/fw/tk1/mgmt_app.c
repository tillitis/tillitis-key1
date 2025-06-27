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
// clang-format off
static const uint8_t allowed_app_digest[32] = {
    0x85, 0x29, 0xe3, 0x25, 0xf5, 0x8d, 0x53, 0x5f,
    0xe1, 0x2a, 0x77, 0x92, 0xe7, 0xdc, 0x4b, 0x4d,
    0x01, 0x85, 0x17, 0xca, 0xfd, 0x54, 0x83, 0xb3,
    0xbb, 0x28, 0x4f, 0xa1, 0x98, 0x5f, 0x9e, 0x56,
};
// clang-format on

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
