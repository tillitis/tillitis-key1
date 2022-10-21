/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

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

	/* Clear all RAM */
	li a0, 0x40000000 // RAM base
	li a1, 0x40020000 // To end of SRAM
clear:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, clear

        /* init stack to right under where we load app at 0x40010000 */
        li sp, 0x4000fff0

	/* copy data section */
	la a0, _sidata
	la a1, _sdata
	la a2, _edata
	bge a1, a2, end_init_data

loop_init_data:
	lw a3, 0(a0)
	sw a3, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	blt a1, a2, loop_init_data

end_init_data:
	/* zero-init bss section */
	la a0, _sbss
	la a1, _ebss
	bge a0, a1, end_init_bss

loop_init_bss:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, loop_init_bss

end_init_bss:
	call main

loop:
	j loop
