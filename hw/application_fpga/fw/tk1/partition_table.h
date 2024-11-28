// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PARTITION_TABLE_H
#define PARTITION_TABLE_H

#include <stdint.h>

/* ----	Flash	----		----		*/
/* name		size		start addr	*/
/* ----		----		----		*/
/* bitstream	128KiB		0x00		*/
/* ----		----		----		*/
/* Partition	64KiB		0x20000		*/
/* ----		----		----		*/
/* Pre load	128KiB		0x30000		*/
/* ----		----		----		*/
/* storage 1	128KiB		0x50000		*/
/* storage 2	128KiB		0x70000		*/
/* storage 3	128KiB		0x90000		*/
/* storage 4	128KiB		0xB0000		*/
/* ----		----		----		*/

/* To simplify all blocks are aligned with the 64KiB blocks on the W25Q80DL
 * flash. */

#define PART_TABLE_VERSION 1

#define ADDR_BITSTREAM 0UL
#define SIZE_BITSTREAM 0x20000UL // 128KiB

#define ADDR_PARTITION_TABLE (ADDR_BITSTREAM + SIZE_BITSTREAM)
#define SIZE_PARTITION_TABLE                                                   \
	0x10000UL // 64KiB, 60 KiB reserved, 2 flash pages (2 x 4KiB) for the
		  // partition table

#define ADDR_PRE_LOADED_APP (ADDR_PARTITION_TABLE + SIZE_PARTITION_TABLE)
#define SIZE_PRE_LOADED_APP 0x20000UL // 128KiB

#define ADDR_STORAGE_AREA (ADDR_PRE_LOADED_APP + SIZE_PRE_LOADED_APP)
#define SIZE_STORAGE_AREA 0x20000UL // 128KiB
#define N_STORAGE_AREA 4

#define EMPTY_AREA

/* Partition Table			*/
/*- Table header			*/
/*  - 1 bytes Version			*/
/**/
/*- Management device app		*/
/*  - Status.				*/
/*  - 16 byte random nonce.		*/
/*  - 16 byte authentication digest.	*/
/**/
/*- Pre-loaded device app		*/
/*  - 1 byte status.			*/
/*  - 4 bytes length.			*/
/*  - 16 bytes random nonce.		*/
/*  - 16 bytes authentication digest.	*/
/**/
/*- Device app storage area		*/
/*  - 1 byte status.			*/
/*  - 16 bytes random nonce.		*/
/*  - 16 bytes authentication tag.	*/
/*  - 4 bytes physical start address.	*/
/*  - 4 bytes physical end address.	*/

typedef struct __attribute__((packed)) {
	uint8_t nonce[16];
	uint8_t authentication_digest[16];
} auth_metadata_t;

typedef struct __attribute__((packed)) {
	uint8_t status;
	auth_metadata_t auth;
} management_app_metadata_t;

typedef struct __attribute__((packed)) {
	uint8_t status;
	uint32_t size;
	auth_metadata_t auth;
} pre_loaded_app_metadata_t;

typedef struct __attribute__((packed)) {
	uint8_t status;
	auth_metadata_t auth;
	uint32_t addr_start;
	uint32_t size;
} app_storage_area_t;

typedef struct __attribute__((packed)) {
	uint8_t version;
} table_header_t;

typedef struct __attribute__((packed)) {
	table_header_t header;
	management_app_metadata_t mgmt_app_data;
	pre_loaded_app_metadata_t pre_app_data;
	app_storage_area_t app_storage[N_STORAGE_AREA];
} partition_table_t;

int part_table_read(partition_table_t *part_table);
int part_table_write(partition_table_t *part_table);

#endif
