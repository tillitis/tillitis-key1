/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

// This firmware copies an app from ROM to app RAM.
// The app continuosly calls the SET_LED syscall in firmware (main.c).
//
#include "../tk1_mem.h"
#include "custom_ops.S"  // PicoRV32 custom instructions

#define illegal_insn() .word 0

#define FW_SP_STORAGE (TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE - 4)
#define FW_STACK_TOP (TK1_MMIO_FW_RAM_BASE + TK1_MMIO_FW_RAM_SIZE - 2*4)

#define LED_RED (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE (1 << TK1_MMIO_TK1_LED_B_BIT)

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
	beq x4, t4, irq_source_ok
unexpected_irq_source:
	illegal_insn()
	j unexpected_irq_source
irq_source_ok:

	// Save app stack pointer. App is responsible for saving the rest of
	// the registers.
	la t0, FW_SP_STORAGE
	sw sp, 0(t0)

	// Setup firmware stack pointer
	la sp, FW_STACK_TOP

	// Run syscall handler
	call syscall_handler

	// Restore app stack pointer
	la t0, FW_SP_STORAGE
	lw sp, 0(t0)

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
	// x3 (interrupt return address) assumed preserved
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

//
// Init
//
	.=0x100
init:
	li t0, TK1_MMIO_TK1_LED
	li t1, LED_RED
	sw t1, 0(t0)

	// Enable IRQs
	li t0, 0x7fffffff // IRQ31 mask
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
#define SYSCALL_SET_LED 10

	.=0x1000
app_start:
	// Set stack pointer to end of app RAM
	li sp, 0x4001fffc

app_loop:
	li a0, SYSCALL_SET_LED
	li a1, LED_GREEN
	call app_syscall
	call app_delay

	li a0, SYSCALL_SET_LED
	li a1, LED_BLUE
	call app_syscall
	call app_delay

	j app_loop

app_syscall:
	// Save registers to stack
	addi sp, sp, -32*4
	sw x0, 0*4(sp)
	sw x1, 1*4(sp)
	// x2 (sp) is assumed to be preserved by the interrupt handler.
	sw x3, 3*4(sp)
	sw x4, 4*4(sp)
	sw x5, 5*4(sp)
	sw x6, 6*4(sp)
	sw x7, 7*4(sp)
	sw x8, 8*4(sp)
	sw x9, 9*4(sp)
	// x10 (a0) will contain syscall return value. And should not be saved.
	sw x11, 11*4(sp)
	sw x12, 12*4(sp)
	sw x13, 13*4(sp)
	sw x14, 14*4(sp)
	sw x15, 15*4(sp)
	sw x16, 16*4(sp)
	sw x17, 17*4(sp)
	sw x18, 18*4(sp)
	sw x19, 19*4(sp)
	sw x20, 20*4(sp)
	sw x21, 21*4(sp)
	sw x22, 22*4(sp)
	sw x23, 23*4(sp)
	sw x24, 24*4(sp)
	sw x25, 25*4(sp)
	sw x26, 26*4(sp)
	sw x27, 27*4(sp)
	sw x28, 28*4(sp)
	sw x29, 29*4(sp)
	sw x30, 30*4(sp)
	sw x31, 31*4(sp)

	// Trigger syscall interrupt
	li t0, (1 << 31)
	and t0, a0, t0
	li t1, 0xe1000000 // Syscall interrupt trigger address
	sw zero, 0(t1) // Trigger interrupt

	// Restore registers from stack
	lw x0, 0*4(sp)
	lw x1, 1*4(sp)
	// x2 (sp) is assumed to be preserved by the interrupt handler.
	lw x3, 3*4(sp)
	lw x4, 4*4(sp)
	lw x5, 5*4(sp)
	lw x6, 6*4(sp)
	lw x7, 7*4(sp)
	lw x8, 8*4(sp)
	lw x9, 9*4(sp)
	// x10 (a0) contains syscall return value. And should not be destroyed.
	lw x11, 11*4(sp)
	lw x12, 12*4(sp)
	lw x13, 13*4(sp)
	lw x14, 14*4(sp)
	lw x15, 15*4(sp)
	lw x16, 16*4(sp)
	lw x17, 17*4(sp)
	lw x18, 18*4(sp)
	lw x19, 19*4(sp)
	lw x20, 20*4(sp)
	lw x21, 21*4(sp)
	lw x22, 22*4(sp)
	lw x23, 23*4(sp)
	lw x24, 24*4(sp)
	lw x25, 25*4(sp)
	lw x26, 26*4(sp)
	lw x27, 27*4(sp)
	lw x28, 28*4(sp)
	lw x29, 29*4(sp)
	lw x30, 30*4(sp)
	lw x31, 31*4(sp)
	addi sp, sp, 32*4

	ret

app_delay:
        li t5, 0x100000
app_delay_dec:
        addi t5, t5, -1
        bne t5, zero, app_delay_dec
        ret

	.align 4
app_end:

