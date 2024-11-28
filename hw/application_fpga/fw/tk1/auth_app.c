// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "auth_app.h"
#include "../tk1_mem.h"
#include "blake2s/blake2s.h"
#include "lib.h"
#include "partition_table.h"
#include "rng.h"

#include <stdbool.h>
#include <stdint.h>

static volatile uint32_t *cdi = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;

/* Calculates the authentication digest based on a supplied nonce and the CDI.
 * Requires that the CDI is already calculated and stored */
static void calculate_auth_digest(uint8_t *nonce, uint8_t *auth_digest)
{
	/* TODO: Check so the CDI is non-zero? */

	blake2s_ctx ctx = {0};

	// Generate a 16 byte authentication digest
	blake2s_init(&ctx, 16, NULL, 0);
	blake2s_update(&ctx, (const void *)cdi, 32);
	blake2s_update(&ctx, nonce, 16);
	blake2s_final(&ctx, auth_digest);
}

/* Generates a 16 byte nonce */
static void generate_nonce(uint32_t *nonce)
{

	for (uint8_t i = 0; i < 4; i++) {
		nonce[i] = rng_get_word();
	}
	return;
}
/* Returns the authentication digest and random nonce. Requires that the CDI is
 * already calculated and stored */
void auth_app_create(auth_metadata_t *auth_table)
{
	uint8_t nonce[16];
	uint8_t auth_digest[16];

	generate_nonce((uint32_t *)nonce);

	calculate_auth_digest(nonce, auth_digest);

	memcpy_s(auth_table->authentication_digest, 16, auth_digest, 16);
	memcpy_s(auth_table->nonce, 16, nonce, 16);

	return;
}

bool auth_app_authenticate(auth_metadata_t *auth_table)
{
	uint8_t auth_digest[16];

	calculate_auth_digest(auth_table->nonce, auth_digest);

	if (memeq(auth_digest, auth_table->authentication_digest, 16)) {
		return true;
	}

	return false;
}
