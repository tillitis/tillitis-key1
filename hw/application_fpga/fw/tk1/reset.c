// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <blake2s/blake2s.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "memcheck.h"
#include "reset.h"

// clang-format off
static volatile uint32_t *cdi           = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
static volatile uint32_t *system_reset  = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
static volatile struct reset *resetinfo = (volatile struct reset *)TK1_MMIO_RESETINFO_BASE;
// clang-format on

int reset(struct user_reset *userreset, size_t nextlen)
{
	if (!in_app_ram(userreset, sizeof(struct reset))) {
		return -1;
	}

	if (nextlen > sizeof(resetinfo->next_app_data)) {
		return -1;
	}

	// Fill in firmware's reset data (preserved over the system
	// reset) with:
	//
	// - allowed app digest from caller
	// - measured id = blake2s(CDI of current running app, measured id seed
	//   from caller)
	// - next app data from caller
	(void)memset((void *)resetinfo, 0, sizeof(*resetinfo));
	resetinfo->type = userreset->type;

	memcpy((void *)resetinfo->app_digest, userreset->app_digest,
	       sizeof(resetinfo->app_digest));

	resetinfo->mask = userreset->mask;

	// Mix in CDI of currently running app and measured_id_seed into
	// measured_id.
	int rc = blake2s((void *)resetinfo->measured_id, RESET_DIGEST_SIZE,
			 (void *)cdi, 32, (void *)userreset->measured_id_seed,
			 RESET_DIGEST_SIZE);
	assert(rc == 0);

	memcpy((void *)resetinfo->next_app_data, userreset->next_app_data,
	       nextlen);

	// Do the actual reset.
	*system_reset = 1;

	// Should not be reached.
	assert(1 == 2);

	__builtin_unreachable();
}

int reset_data(uint8_t *next_app_data)
{
	if (!in_app_ram(next_app_data, RESET_DATA_SIZE)) {
		return -1;
	}

	memcpy(next_app_data, (void *)resetinfo->next_app_data,
	       RESET_DATA_SIZE);

	return 0;
}
