// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PARTITION_TABLE_H
#define PARTITION_TABLE_H

#include <stdint.h>

// ----	Flash	----		----
// name		size		start addr
// ----		----		----
// bitstream	128KiB		0x00
// ----		----		----
// Partition	64KiB		0x20000
// ----		----		----
// Pre load 1	128KiB		0x30000
// Pre load 2	128KiB		0x50000
// ----		----		----
// storage 1	128KiB		0x70000
// storage 2	128KiB		0x90000
// storage 3	128KiB		0xB0000
// storage 4	128KiB		0xD0000
// ----		----		----
// Partition2   64KiB		0xf0000

// To simplify all blocks are aligned with the 64KiB blocks on the
// W25Q80DL flash.

#define PART_TABLE_VERSION 1

#define ADDR_BITSTREAM 0UL
#define SIZE_BITSTREAM 0x20000UL // 128KiB

#define ADDR_PARTITION_TABLE_0 (ADDR_BITSTREAM + SIZE_BITSTREAM)
#define ADDR_PARTITION_TABLE_1 0xf0000
#define SIZE_PARTITION_TABLE                                                   \
	0x10000UL // 64KiB, 60 KiB reserved, 2 flash pages (2 x 4KiB) for the
		  // partition table

#define N_PRELOADED_APP 2
#define ADDR_PRE_LOADED_APP_0 (ADDR_PARTITION_TABLE_0 + SIZE_PARTITION_TABLE)
#define SIZE_PRE_LOADED_APP 0x20000UL // 128KiB

#define ADDR_STORAGE_AREA                                                      \
	(ADDR_PRE_LOADED_APP_0 + (N_PRELOADED_APP * SIZE_PRE_LOADED_APP))
#define SIZE_STORAGE_AREA 0x20000UL // 128KiB
#define N_STORAGE_AREA 4

#define PART_CHECKSUM_SIZE 32

enum part_status {
	PART_SLOT0_INVALID = 1,
};

// Partition Table
// ----------------------------------------------------------------------
// - Table header
// - 1 bytes Version
//
// - Pre-loaded device app 1
//   - 4 bytes length.
//   - 32 bytes digest.
//   - 64 bytes signature.
//
// - Pre-loaded device app 2
//   - 4 bytes length.
//   - 32 bytes digest.
//   - 64 bytes signature.
//
// - Device app storage area
//   - 1 byte status.
//   - 16 bytes random nonce.
//   - 16 bytes authentication tag.
//
// - Checksum over the above

struct auth_metadata {
	uint8_t nonce[16];
	uint8_t authentication_digest[16];
} __attribute__((packed));

struct pre_loaded_app_metadata {
	uint32_t size;
	uint8_t digest[32];
	uint8_t signature[64];
} __attribute__((packed));

struct app_storage_area {
	uint8_t status;
	struct auth_metadata auth;
} __attribute__((packed));

struct table_header {
	uint8_t version;
} __attribute__((packed));

struct partition_table {
	struct table_header header;
	struct pre_loaded_app_metadata pre_app_data[N_PRELOADED_APP];
	struct app_storage_area app_storage[N_STORAGE_AREA];
} __attribute__((packed));

struct partition_table_storage {
	struct partition_table table;
	uint8_t checksum[PART_CHECKSUM_SIZE]; // Helps detect flash problems
} __attribute__((packed));

enum part_status part_get_status(void);
int part_table_read(struct partition_table_storage *storage);
int part_table_write(struct partition_table_storage *storage);

#endif
