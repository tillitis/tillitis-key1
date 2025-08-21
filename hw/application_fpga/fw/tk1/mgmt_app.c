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
    0x40, 0x7f, 0x58, 0xbe, 0x39, 0xcf, 0xae, 0xaf,
    0x43, 0xa0, 0x75, 0x90, 0x4d, 0x43, 0xa3, 0x2e,
    0xa1, 0x5f, 0x4c, 0x1b, 0x6a, 0xf3, 0x69, 0x4d,
    0x74, 0x05, 0x21, 0x63, 0xa0, 0xd3, 0x69, 0x34,
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
