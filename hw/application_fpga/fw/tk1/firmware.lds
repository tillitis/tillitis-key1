/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY
{
	/* TODO ROM size should be adjusted, RAM should be ok. */
	ROM (rx)    : ORIGIN = 0x00000000, LENGTH = 0x20000 /* 128 KB */
	RAM (rwx)   : ORIGIN = 0x40000000, LENGTH = 0x20000 /* 128 KB */
}

SECTIONS
{
	.text.init :
	{
		*(.text.init)
	} >ROM

	.htif :
	{
		. = ALIGN(0x00000000);
		*(.htif)
	} >ROM

	.text :
	{
		. = ALIGN(4);
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.rodata)         /* .rodata sections (constants, strings, etc.) */
		*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
		*(.srodata)        /* .rodata sections (constants, strings, etc.) */
		*(.srodata*)       /* .rodata* sections (constants, strings, etc.) */
		. = ALIGN(4);
		_etext = .;
		_sidata = _etext;
	} >ROM

	.data : AT (_etext)
	{
		. = ALIGN(4);
		_sdata = .;
		. = ALIGN(4);
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */
		*(.sdata)           /* .sdata sections */
		*(.sdata*)          /* .sdata* sections */
		. = ALIGN(4);
		_edata = .;
	} >RAM

	/* Uninitialized data section */
	.bss :
	{
		. = ALIGN(4);
		_sbss = .;
		*(.bss)
		*(.bss*)
		*(.sbss)
		*(.sbss*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;
	} >RAM
	/* Init stack to _ebss + size */
}
