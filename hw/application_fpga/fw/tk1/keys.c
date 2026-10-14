// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <blake2s/blake2s.h>
#include <stdbool.h>
#include <stdint.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "rng.h"

// clang-format off
static volatile uint32_t *uds              = (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
static volatile uint32_t *timer            = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler  = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status     = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl       = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
// clang-format on

static bool keys_initialzed = false;

bool keys_is_init(void)
{
	return keys_initialzed;
}

// Generates keys based on the UDS.
void keys_generate(uint8_t *cdi_key, uint8_t *part_key)
{
	if (keys_initialzed) {
		return;
	}

	uint32_t local_uds[8];
	blake2s_ctx b2s_ctx = {0};

	// Prepare to sleep a random number of cycles before reading out UDS
	*timer_prescaler = 1;
	uint32_t rnd_sleep = rng_get_word();
	// Up to 65536 cycles
	rnd_sleep &= 0xffff;
	*timer = (uint32_t)(rnd_sleep == 0 ? 1 : rnd_sleep);
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	while (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
	}

	// Initialize the BLAKE2s hash function with the UDS as key.
	// This means UDS will live for a short while on the firmware
	// stack, fw_ram.
	wordcpy_s(local_uds, 8, (void *)uds, 8);

	static const uint8_t cdi_info[] = "kdf/cdi_key";
	blake2s_init(&b2s_ctx, 32, local_uds, 32);
	blake2s_update(&b2s_ctx, cdi_info, sizeof(cdi_info) - 1);
	blake2s_final(&b2s_ctx, cdi_key);

	static const uint8_t part_table_info[] = "kdf/p_table_key";

	blake2s_init(&b2s_ctx, 32, local_uds, 32);
	(void)secure_wipe(local_uds, sizeof(local_uds));
	blake2s_update(&b2s_ctx, part_table_info, sizeof(part_table_info) - 1);
	blake2s_final(&b2s_ctx, part_key);

	(void)secure_wipe(&b2s_ctx, sizeof(b2s_ctx));
	keys_initialzed = true;
}
