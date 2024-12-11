/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

// This firmwares copies an app from ROM to app RAM. The app triggers both
// IRQ_SYSCALL_HI and IRQ_SYSCALL_LO. One after the other. Finally, the
// app tries to jump firmware. This should result in a trap since the
// app in executing in app mode.
//

#include "custom_ops.S"  // PicoRV32 custom instructions

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
	// If bit 30 is 1: IRQ30 was triggered.

	nop // NOPs are not necessary. Only added to make it easier to find
	nop // when simulating.
	nop
	picorv32_retirq_insn() // Return from interrupt

//
// Init
//
	.=0x20 // Setting location of init to 0x20. Makes it easier to find when
	       // simulating.
init:
	li t0, 0x3fffffff // IRQ31 & IRQ30 mask
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
	li t0, 0xe1000000 // IRQ_SYSCALL_HI (IRQ31) trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.

	li t0, 0xe0000000 // IRQ_SYSCALL_LO (IRQ30) trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.

	jalr zero, 0(zero) // Jumping to firmware. Expecting trap
app_loop:
	j app_loop
	.align 4
app_end:

