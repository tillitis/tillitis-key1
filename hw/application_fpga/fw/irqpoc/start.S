/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

// Example firmware that demonstrates how to raise interrupts
//

#include "custom_ops.S"  // PicoRV32 custom instructions

	.section ".text.init"
	.globl _start
_start:
	j init

	.=0x10 // IRQ handler at fixed address 0x10
irq_handler:
	// PicoRV32 stores the IRQ bitmask in x4.
	// If bit 31 is 1: IRQ31 was triggered.

	nop // NOPs are not necessary. Only added to make it easier to find
	nop // when simulating.
	nop
	picorv32_retirq_insn() // Return from interrupt

	.=0x20 // Setting location of init to 0x20. Makes it easier to find when
	       // simulating.
init:
	li t0, 0x7fffffff // IRQ31 mask
	picorv32_maskirq_insn(zero, t0) // Enable IRQs

	li t0, 0xe1000000 // IRQ31 trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.
loop:
	j loop

	.align 4 // Padding to please makehex.py which requires even 4-byte file
	         // sizes.

