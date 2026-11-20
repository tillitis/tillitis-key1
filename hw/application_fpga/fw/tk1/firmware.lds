/*
 * SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
 * SPDX-License-Identifier: BSD-2-Clause
 */

OUTPUT_ARCH("riscv")
ENTRY(_start)

/* Define stack size */
STACK_SIZE = 3000;

MEMORY
{
	ROM       (rx)  : ORIGIN = 0x00000000, LENGTH = 0x2000  /* 8 KB */
	FWRAM     (rw)  : ORIGIN = 0xd0000000, LENGTH = 0xF00   /* 3840 B */
	RESETINFO (rw)  : ORIGIN = 0xd0000F00, LENGTH = 0x100   /* 256 B (part of FW_RAM area) */
	RAM       (rwx) : ORIGIN = 0x40000000, LENGTH = 0x20000 /* 128 KB */
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
		_stextfw = .;
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.rodata)         /* .rodata sections (constants, strings, etc.) */
		*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
		*(.srodata)        /* .srodata sections (constants, strings, etc.) */
		*(.srodata*)       /* .srodata* sections (constants, strings, etc.) */
		. = ALIGN(4);
		_etextfw = .;
	} >ROM

	.stack (NOLOAD) :
	{
		. = ALIGN(16);
		_sstackfw = .;
		. += STACK_SIZE;
		. = ALIGN(16);
		_estackfw = .;
	} >FWRAM

	.data :
	{
		. = ALIGN(4);
		_sdatafw = .;
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */
		*(.sdata)          /* .sdata sections */
		*(.sdata*)         /* .sdata* sections */
		. = ALIGN(4);
		_edatafw = .;
	} >FWRAM AT>ROM
	_sidatafw = LOADADDR(.data);

	/* Uninitialized data section */
	.bss :
	{
		. = ALIGN(4);
		_sbssfw = .;
		*(.bss)
		*(.bss*)
		*(.sbss)
		*(.sbss*)
		*(COMMON)
		. = ALIGN(4);
		_ebssfw = .;
	} >FWRAM
}

_sfwram = ORIGIN(FWRAM);
_efwram = ORIGIN(FWRAM) + LENGTH(FWRAM);
_sresetinfo = ORIGIN(RESETINFO);
_eresetinfo = ORIGIN(RESETINFO) + LENGTH(RESETINFO);
_sfwram_all = ORIGIN(FWRAM);
_efwram_all = ORIGIN(FWRAM) + LENGTH(FWRAM) + LENGTH(RESETINFO);
_sappram = ORIGIN(RAM);
_eappram = ORIGIN(RAM) + LENGTH(RAM);
