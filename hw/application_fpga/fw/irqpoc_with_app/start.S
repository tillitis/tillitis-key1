/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

// This firmware copies an app from ROM to app RAM. The app triggers IRQ_SYSCALL
// Checks are done to make sure that firmware RAM can/cannot be read depending
// on wether we're executing from firmware, syscall or app.
// Finally the app tries to jump firmware. This should result in a trap since
// the app is executing in app mode.
//

#include "custom_ops.S"  // PicoRV32 custom instructions

#define illegal_insn() .word 0

	.section ".text.init"
	.globl _start
_start:
	j init

//
// IRQ handler
//
	.=0x10 // IRQ handler at fixed address 0x10
irq_handler:
	// PicoRV32 stores the IRQ bitmask in x4.
	// If bit 31 is 1: IRQ31 was triggered.
	li t4, (1 << 31)
	bne x4, t4, unexpected_irq
	// Firmware RAM should be readable from IRQ_SYSCALL
	call check_can_read_test_val_from_fw_ram
	j irq_source_check_done
unexpected_irq:
	illegal_insn()
irq_source_check_done:
	picorv32_retirq_insn() // Return from interrupt

//
// Init
//
	.=0x100
init:
	// Save test value in firmware RAM
	li t0, 0xd0000000
	li t1, 0x5555aaaa
	sw t1, 0(t0)

	// Firmware RAM should be readable from firmware mode
	call check_can_read_test_val_from_fw_ram

	// Enable IRQs
	li t0, 0x7fffffff // IRQ31
	picorv32_maskirq_insn(zero, t0) // Enable IRQs

	// Copy app to App RAM
	la t0, app_start
	la t1, app_end
	li t2, 0x40000000 // 0x40000000: App RAM
copy_app:
	lw t3, 0(t0)
	sw t3, 0(t2)
	addi t0, t0, 4
	addi t2, t2, 4
	bleu t0, t1, copy_app

	// Jump to app
	li t2, 0x40000000 // 0x40000000: App RAM
	jalr zero, 0(t2)

//
// App
//
	.align 4
app_start:
	// Firmware RAM should not be readable from app mode
	call check_cannot_read_test_val_from_fw_ram

	// Raise IRQ_SYSCALL
	li t0, 0xe1000000 // IRQ_SYSCALL (IRQ31) trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.

	jalr zero, 0(zero) // Jumping to firmware. Expecting trap
app_loop:
	j app_loop


check_cannot_read_test_val_from_fw_ram:
	li t0, 0xd0000000
	lw t1, 0(t0)
	li t2, 0
	bne t1, t2, cannot_read_test_val_from_fw_ram_fail
	ret
cannot_read_test_val_from_fw_ram_fail:
	illegal_insn()

check_can_read_test_val_from_fw_ram:
	// Check that saved test value can not be read while in app mode
	li t0, 0xd0000000
	lw t1, 0(t0)
	li t2, 0x5555aaaa
	bne t1, t2, can_read_test_val_from_fw_ram_fail
	ret
can_read_test_val_from_fw_ram_fail:
	illegal_insn()

	.align 4
app_end:

