// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_RESET_H
#define TKEY_RESET_H

#include <stddef.h>
#include <stdint.h>

#define TK1_MMIO_RESETINFO_BASE 0xd0000f00
#define TK1_MMIO_RESETINFO_SIZE 0x100
#define RESET_DIGEST_SIZE 32
#define RESET_DATA_SIZE 184

enum reset_start {
	START_DEFAULT = 0, // Probably cold boot
	START_FLASH0 = 1,
	START_FLASH1 = 2,
	START_FLASH0_VER = 3,
	START_FLASH1_VER = 4,
	START_CLIENT = 5,
	START_CLIENT_VER = 6,
};

#define RESET_NEXT 0x01
#define RESET_SEED 0x02

struct reset {
	enum reset_start type;
	uint8_t mask;
	uint8_t app_digest[RESET_DIGEST_SIZE];
	uint8_t measured_id[RESET_DIGEST_SIZE];
	uint8_t next_app_data[RESET_DATA_SIZE];
} __attribute__((__packed__));

// TODO Use version from tkey-libs when new version has been imported
struct user_reset {
	enum reset_start type;
	uint8_t mask;
	uint8_t app_digest[RESET_DIGEST_SIZE];
	uint8_t measured_id_seed[RESET_DIGEST_SIZE];
	uint8_t next_app_data[RESET_DATA_SIZE];
};

int reset(struct user_reset *userreset, size_t nextlen);
int reset_data(uint8_t *next_app_data);
#endif
