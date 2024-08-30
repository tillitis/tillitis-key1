// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef PARTITION_TABLE_H
#define PARTITION_TABLE_H


#include <stdint.h>

#define PART_TABLE_VER 1

#define ADDR_BITSTREAM 0UL
#define SIZE_BITSTREAM 0x20000UL

#define ADDR_PARTITION_TABLE (ADDR_BITSTREAM + SIZE_BITSTREAM)
#define SIZE_PARTITION_TABLE 0x3000UL /* 3 flash sectors */

#define ADDR_PRE_LOADED_APP (ADDR_PARTITION_TABLE + SIZE_PARTITION_TABLE)
#define SIZE_PRE_LOADED_APP 0x20000UL

#define ADDR_STORAGE_AREA (ADDR_PRE_LOADED_APP + SIZE_PARTITION_TABLE)
#define SIZE_STORAGE_AREA 0x1F000UL
#define N_STORAGE_AREA 4

/*- Table header*/
/*  - 4 bytes Version*/
/**/
/*- Management device app*/
/*  - Status - Registered or not.*/
/*  - 16 byte random nonce.*/
/*  - 16 byte authentication digest.*/
/**/
/*- Pre-loaded device app*/
/*  - 1 byte status - Pre-loaded device app exists or not. Active or not.*/
/*  - 4 bytes length of pre-loaded device app in bytes.*/
/*  - 16 bytes random nonce.*/
/*  - 16 bytes authentication digest.*/
/**/
/*- Device app storage area*/
/*  - 1 byte status - Allocated or not.*/
/*  - 16 bytes random nonce.*/
/*  - 16 bytes authentication tag.*/
/*  - 4 bytes physical start address in the persistent store.*/
/*  - 4 bytes physical end address in the persistent store.*/

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
	management_app_metadata_t mgmt_app_data;
	pre_loaded_app_metadata_t pre_app_data;
	app_storage_area_t app_storage[N_STORAGE_AREA];
} partition_table_t;

int part_table_init(partition_table_t *part_table);
int part_table_write(partition_table_t *part_table);

#endif
