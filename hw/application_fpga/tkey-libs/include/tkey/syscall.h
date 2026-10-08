// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>

#ifndef TKEY_SYSCALL_H
#define TKEY_SYSCALL_H

#define RESET_DIGEST_SIZE 32
#define RESET_DATA_SIZE 184

// Needs to be held synchronized with syscall_num.h in firmware.
enum reset_start {
	START_FLASH0 = 0,
	START_FLASH1_VER = 1,
	START_CLIENT = 2,
	START_CLIENT_VER = 3,
};

#define RESET_NEXT 0x01
#define RESET_SEED 0x02

struct reset {
	enum reset_start type;
	uint8_t mask;
	uint8_t app_digest[RESET_DIGEST_SIZE];
	uint8_t measured_id_seed[RESET_DIGEST_SIZE];
	uint8_t next_app_data[RESET_DATA_SIZE];
};

// Needs to be held synchronized with syscall_num.h in firmware.
enum syscall_num {
	TK1_SYSCALL_RESET = 1,
	TK1_SYSCALL_ALLOC_AREA = 2,
	TK1_SYSCALL_DEALLOC_AREA = 3,
	TK1_SYSCALL_WRITE_DATA = 4,
	TK1_SYSCALL_READ_DATA = 5,
	TK1_SYSCALL_ERASE_DATA = 6,
	TK1_SYSCALL_GET_VIDPID = 7,
	TK1_SYSCALL_PRELOAD_STORE = 8,
	TK1_SYSCALL_PRELOAD_STORE_FIN = 9,
	TK1_SYSCALL_PRELOAD_DELETE = 10,
	TK1_SYSCALL_PRELOAD_GET_METADATA = 11,
	TK1_SYSCALL_RESERVED = 12,
	TK1_SYSCALL_STATUS = 13,
	TK1_SYSCALL_GET_RESET_DATA = 14,
	TK1_SYSCALL_PRELOAD_SET_PUBKEY = 15,
	TK1_SYSCALL_ERASE_AREA = 16,
};

int syscall(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3);
int sys_reset(struct reset *rst, size_t len);
int sys_get_reset_data(uint8_t next_app_data[RESET_DATA_SIZE]);
int sys_alloc(void);
int sys_dealloc(void);
int sys_write(uint32_t offset, void *buf, size_t len);
int sys_read(uint32_t offset, void *buf, size_t len);
int sys_erase(uint32_t offset, size_t len);
int sys_get_vidpid(void);
int sys_preload_delete(void);
int sys_preload_store(uint32_t offset, void *app, size_t len);
int sys_preload_store_fin(size_t len, uint8_t digest[32],
			  uint8_t signature[64]);
int sys_preload_get_metadata(uint8_t digest[32], uint8_t signature[64],
			     uint8_t pubkey[32]);
int sys_preload_set_pubkey(uint8_t pubkey[32]);
int sys_status(void);
int sys_erase_area(uint8_t area);
#endif
