// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/syscall.h>
#include <tkey/tk1_mem.h>

#include "partition_table.h"
#include "preload_app.h"
#include "reset.h"
#include "storage.h"
#include "string.h"

// clang-format off
static volatile uint32_t *udi           = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
// clang-format on

extern struct partition_table_storage part_table_storage;
extern uint8_t part_status;

#define CDI_WORDS 8
#define RESET_DIGEST_SIZE 32
#define RESET_DATA_SIZE 184

// clang-format off
static volatile uint32_t *cdi           = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
static volatile struct reset *resetinfo = (volatile struct reset *)TK1_MMIO_RESETINFO_BASE;
// clang-format on

uint32_t cdi_clean[CDI_WORDS] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

static void firmware_assert_fail(void) __attribute__((noreturn));
static void firmware_assert_fail(void)
{
	// Clear CDI
	wordcpy_s_volatile((void *)cdi, CDI_WORDS, cdi_clean, CDI_WORDS);
	// Clear reset data
	(void)memset((void *)resetinfo->app_digest, 0, RESET_DIGEST_SIZE);
	(void)memset((void *)resetinfo->measured_id_seed, 0, RESET_DIGEST_SIZE);
	(void)memset((void *)resetinfo->next_app_data, 0, RESET_DATA_SIZE);

	// clang-format off

	// Clear fw stack + fw data + fw bss
	asm volatile(
			"la a0, _sfwram;"
			"la a1, _efwram;"
			"loop_assert_fail:;"
			"sw zero, 0(a0);"
			"addi a0, a0, 4;"
			"blt a0, a1, loop_assert_fail;" ::
			: "memory");

	// Force illegal instruction to halt CPU
	for (;;) {
		asm volatile("unimp");
		asm volatile("unimp");
		asm volatile("unimp");
		asm volatile("unimp");
	}

	// clang-format on
	__builtin_unreachable();
}

int32_t syscall_handler(uint32_t number, uint32_t arg1, uint32_t arg2,
			uint32_t arg3)
{
	switch (number) {
	case TK1_SYSCALL_RESET:
		return reset((struct reset *)arg1, (size_t)arg2);
		break;

	case TK1_SYSCALL_ALLOC_AREA:
		if (storage_allocate_area(&part_table_storage) < 0) {
			debug_puts("couldn't allocate storage area\n");
			return -1;
		}
		return 0;

	case TK1_SYSCALL_DEALLOC_AREA:
		if (storage_deallocate_area(&part_table_storage) < 0) {
			debug_puts("couldn't deallocate storage area\n");
			return -1;
		}
		return 0;

	case TK1_SYSCALL_WRITE_DATA:
		if (storage_write_data(&part_table_storage.table, arg1,
				       (uint8_t *)arg2, arg3) < 0) {
			debug_puts("couldn't write storage area\n");
			return -1;
		}
		return 0;

	case TK1_SYSCALL_READ_DATA:
		if (storage_read_data(&part_table_storage.table, arg1,
				      (uint8_t *)arg2, arg3) < 0) {
			debug_puts("couldn't read storage area\n");
			return -1;
		}
		return 0;

	case TK1_SYSCALL_ERASE_DATA:
		if (storage_erase_sector(&part_table_storage.table, arg1,
					 arg2) < 0) {
			debug_puts("couldn't erase storage area\n");
			return -1;
		}
		return 0;

	case TK1_SYSCALL_GET_VIDPID:
		// UDI is 2 words: VID/PID & serial. Return just the
		// first word. Serial is kept secret to the device
		// app.
		return udi[0];

	case TK1_SYSCALL_PRELOAD_ERASE:
		// arg1 struct metadata
		// always using slot 1
		return preload_erase_app(&part_table_storage, 1);

	case TK1_SYSCALL_PRELOAD_STORE:
		// arg1 offset
		// arg2 data
		// arg3 size
		// always using slot 1
		return preload_store(&part_table_storage.table, arg1,
				     (uint8_t *)arg2, arg3, 1);

	case TK1_SYSCALL_PRELOAD_SET_METADATA:
		// arg1 struct metadata
		// always using slot 1
		return preload_set_metadata(&part_table_storage,
					    (struct app_metadata *)arg1, 1);

	case TK1_SYSCALL_PRELOAD_GET_METADATA:
		// arg1 struct metadata
		// always using slot 1
		return preload_get_metadata(&part_table_storage.table,
					    (struct app_metadata *)arg1, 1);

	case TK1_SYSCALL_STATUS:
		return part_get_status();

	case TK1_SYSCALL_GET_RESET_DATA:
		// arg1 next_app_data
		return reset_get_reset_data((uint8_t *)arg1);

	case TK1_SYSCALL_ERASE_AREA:
		// arg1 area
		return storage_erase_area(&part_table_storage, (uint8_t)arg1);

	case TK1_SYSCALL_ASSERT_FAIL:
		firmware_assert_fail();
		// not reached
		break;

	default:
		firmware_assert_fail();
	}

	firmware_assert_fail();
	return -1; // This should never run
}
