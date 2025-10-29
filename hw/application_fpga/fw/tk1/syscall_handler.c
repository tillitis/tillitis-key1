// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "partition_table.h"
#include "preload_app.h"
#include "reset.h"
#include "storage.h"
#include "syscall_num.h"

// clang-format off
static volatile uint32_t *udi           = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
// clang-format on

extern struct partition_table_storage part_table_storage;
extern uint8_t part_status;

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

	case TK1_SYSCALL_PRELOAD_DELETE:
		return preload_delete(&part_table_storage, 1);

	case TK1_SYSCALL_PRELOAD_STORE:
		// arg1 offset
		// arg2 data
		// arg3 size
		// always using slot 1
		return preload_store(&part_table_storage.table, arg1,
				     (uint8_t *)arg2, arg3, 1);

	case TK1_SYSCALL_PRELOAD_STORE_FIN:
		// arg1 app_size
		// arg2 app_digest
		// arg3 app_signature
		// always using slot 1
		return preload_store_finalize(&part_table_storage, arg1,
					      (uint8_t *)arg2, (uint8_t *)arg3,
					      1);

	case TK1_SYSCALL_PRELOAD_GET_DIGSIG:
		return preload_get_digsig(&part_table_storage.table,
					  (uint8_t *)arg1, (uint8_t *)arg2, 1);

	case TK1_SYSCALL_STATUS:
		return part_get_status();

	case TK1_SYSCALL_GET_APP_DATA:
		// arg1 next_app_data
		return reset_data((uint8_t *)arg1);

	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}
