// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include "rng.h"
#include <tkey/tk1_mem.h>

#include <stdint.h>

// clang-format off
static volatile uint32_t *trng_status  = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
// clang-format on

uint32_t rng_get_word(void)
{
	while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
	}
	return *trng_entropy;
}

uint32_t rng_xorwow(uint32_t state, uint32_t acc)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	state += acc;
	return state;
}
