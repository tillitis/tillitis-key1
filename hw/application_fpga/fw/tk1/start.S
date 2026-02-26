// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/tk1_mem.h>

#ifdef QEMU_SYSCALL

#define picorv32_retirq_insn(...) \
	mv ra, x3; \
	ret

#else

#include "picorv32/custom_ops.S"  // PicoRV32 custom instructions

#endif

#define illegal_insn() .word 0

	// Variables in bss
	.lcomm irq_ret_addr, 4
	.lcomm app_sp, 4

	.section ".text.init"
	.globl _start
_start:
	j init

// IRQ handler
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
	la t0, irq_ret_addr
	sw x3, 0(t0)

	// Save app stack pointer. App is responsible for saving the rest of
	// the registers.
	la t0, app_sp
	sw sp, 0(t0)

	// Setup firmware stack pointer
	la sp, _estack

	// Run syscall handler
	call syscall_handler

	// Restore app stack pointer
	la t0, app_sp
	lw sp, 0(t0)

	// Restore interrupt return address (x3)
	la t0, irq_ret_addr
	lw x3, 0(t0)

	// Verify that interrupt return address (x3) is in app RAM
	li t0, TK1_RAM_BASE // 0x40000000
	blt x3, t0, x3_invalid
	li t0, TK1_RAM_BASE + TK1_RAM_SIZE
	bge x3, t0, x3_invalid
	j x3_valid
x3_invalid:
	illegal_insn()
	j x3_invalid
x3_valid:

	// Remove data left over from the syscall handling
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

// Init

	.=0xb4
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

	// Clear FW_RAM which includes .bss and .data
	la a0, _sfwram
	la a1, _efwram
clear:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, clear

	// Init stack
	la sp, _estack

	call main

loop:
	j loop
