// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

	.section ".text.init"
	.globl _start
_start:
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

	// Clear all RAM
	li a0, 0x40000000 // TK1_RAM_BASE
	li a1, 0x40020000 // TK1_RAM_BASE + TK1_RAM_SIZE
clear:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, clear

        // NOTE WELL
        // For testfw we init stack at top of RAM
        //
        li sp, 0x40020000 // TK1_RAM_BASE + TK1_RAM_SIZE

	call main

loop:
	j loop
