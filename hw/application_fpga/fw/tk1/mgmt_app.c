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
    0xb6, 0x86, 0x1b, 0x26, 0xef, 0x69, 0x77, 0x12, 0xed, 0x6c, 0xca,
    0xe8, 0x35, 0xb4, 0x5c, 0x01, 0x07, 0x71, 0xab, 0xce, 0x3f, 0x30,
    0x79, 0xda, 0xe6, 0xf9, 0xee, 0x4b, 0xe2, 0x06, 0x95, 0x33,
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
