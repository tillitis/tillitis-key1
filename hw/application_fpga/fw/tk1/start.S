/*
 * Copyright (C) 2022, 2023, 2024, 2025 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

#include "../tk1_mem.h"
#include "picorv32/custom_ops.S"  // PicoRV32 custom instructions

#define illegal_insn() .word 0

#define APP_SP_STORAGE (TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE - 4)
#define IRQ_RET_ADDR_STORAGE (TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE - 2*4)
#define FW_STACK_TOP (TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE - 3*4)

	.section ".text.init"
	.globl _start
_start:
	j init

/*
 * IRQ handler
*/
	.=0x10
irq_handler:
	// PicoRV32 stores the IRQ bitmask in x4.
	// If bit 31 is 1: IRQ31 was triggered.
	li t4, (1 << 31)
	beq x4, t4, irq_source_ok
unexpected_irq_source:
	illegal_insn()
	j unexpected_irq_source
irq_source_ok:

	// Save interrupt return address (x3)
	li t0, IRQ_RET_ADDR_STORAGE
	sw x3, 0(t0)

	// Save app stack pointer. App is responsible for saving the rest of
	// the registers.
	li t0, APP_SP_STORAGE
	sw sp, 0(t0)

	// Setup firmware stack pointer
	li sp, FW_STACK_TOP

	// Run syscall handler
	call syscall_handler

	// Restore app stack pointer
	li t0, APP_SP_STORAGE
	lw sp, 0(t0)

	// Restore interrupt return address (x3)
	li t0, IRQ_RET_ADDR_STORAGE
	lw x3, 0(t0)

	// Verify that interrupt return address (x3) is in app RAM
	li t0, TK1_RAM_BASE // 0x40000000
	blt x3, t0, x3_invalid
	li t0, TK1_RAM_BASE + TK1_RAM_SIZE // 0x40020000
	bge x3, t0, x3_invalid
	j x3_valid
x3_invalid:
	illegal_insn()
	j x3_invalid
x3_valid:

	// Remove data left over from the syscall handling
	mv x0, zero
	mv x1, zero
	// x2 (sp) is assumed to be preserved by the interrupt handler
	// x3 (interrupt return address) need to be preserved
	mv x4, zero
	mv x5, zero
	mv x6, zero
	mv x7, zero
	mv x8, zero
	mv x9, zero
	// x10 (a0) contains syscall return value. And should not be destroyed.
	mv x11, zero
	mv x12, zero
	mv x13, zero
	mv x14, zero
	mv x15, zero
	mv x16, zero
	mv x17, zero
	mv x18, zero
	mv x19, zero
	mv x20, zero
	mv x21, zero
	mv x22, zero
	mv x23, zero
	mv x24, zero
	mv x25, zero
	mv x26, zero
	mv x27, zero
	mv x28, zero
	mv x29, zero
	mv x30, zero
	mv x31, zero

	picorv32_retirq_insn() // Return from interrupt

/*
 * Init
*/
	.=0x100
init:
	li x1, 0
	li x2, 0
	li x3, 0
	li x4, 0
	li x5, 0
	li x6, 0
	li x7, 0
	li x8, 0
	li x9, 0
	li x10,0
	li x11,0
	li x12,0
	li x13,0
	li x14,0
	li x15,0
	li x16,0
	li x17,0
	li x18,0
	li x19,0
	li x20,0
	li x21,0
	li x22,0
	li x23,0
	li x24,0
	li x25,0
	li x26,0
	li x27,0
	li x28,0
	li x29,0
	li x30,0
	li x31,0

	/* Clear FW_RAM */
	li a0, 0xd0000000 // TK1_MMIO_FW_RAM_BASE
	li a1, 0xd0000800 // TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE
clear:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, clear

        /*
         * Init stack at top of fw_ram.
         */
        li sp, 0xd0000800 // 2 kiB (TK1_MMIO_FW_RAM_SIZE)

	call main

loop:
	j loop
