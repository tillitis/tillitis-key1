// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>

#ifndef TKEY_SYSCALL_H
#define TKEY_SYSCALL_H

#define RESET_DIGEST_SIZE 32
#define RESET_DATA_SIZE 220

// Needs to be held synchronized with syscall_num.h in firmware.
enum reset_start {
	START_DEFAULT = 0, // Probably cold boot
	START_FLASH0 = 1,
	START_FLASH1 = 2,
	START_FLASH0_VER = 3,
	START_FLASH1_VER = 4,
	START_CLIENT = 5,
	START_CLIENT_VER = 6,
};

struct reset {
	enum reset_start type;
	uint8_t app_digest[RESET_DIGEST_SIZE];
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
	TK1_SYSCALL_PRELOAD_GET_DIGSIG = 11,
	TK1_SYSCALL_REG_MGMT = 12,
	TK1_SYSCALL_STATUS = 13,
	TK1_SYSCALL_GET_APP_DATA = 14,
};

int syscall(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3);
int sys_reset(struct reset *rst, size_t len);
int sys_reset_data(uint8_t next_app_data[RESET_DATA_SIZE]);
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
int sys_get_digsig(uint8_t digest[32], uint8_t signature[64]);
int sys_status(void);
#endif
