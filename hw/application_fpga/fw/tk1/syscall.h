// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef SYSCALL_H
#define SYSCALL_H

#include "partition_table.h"

#include <stdint.h>
#include <stddef.h>


typedef struct {
	uint8_t syscall_no;
	uint32_t offset;
	uint8_t *data;
	size_t size;
	uint32_t *ctx;
	int ret_value;
} syscall_t;

enum syscall_cmd {
	BLAKE2S = 0,
	ALLOC_AREA,
	DEALLOC_AREA,
	WRITE_DATA,
	READ_DATA,
	ERASE_DATA,
	PRELOAD_STORE,
	PRELOAD_STORE_FINALIZE,
	PRELOAD_DELETE,
	MGMT_APP_REGISTER,
	MGMT_APP_UNREGISTER,
};

void syscall(volatile syscall_t *ctx);
/*int syscall(syscall_t *ctx);*/

#endif
