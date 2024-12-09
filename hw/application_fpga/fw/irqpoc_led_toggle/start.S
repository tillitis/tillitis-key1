/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
*/

// Triggers both interrupts, one after the other, in an endless loop. The
// interrupt handler updates the LED color depending on which interrupt
// was triggered.
// The LED color will alterate between blue and green.
//
// | IRQ number | Color |
// |------------|-------|
// | 30         | Blue  |
// | 31         | Green |
// | Unexpected | Red   |
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
	// If bit 30 is 1: IRQ30 was triggered.
irq30_check:
	li t4, (1 << 30)
	bne x4, t4, irq31_check
	call led_blue
	call delay
	j irq_source_check_done
irq31_check:
	li t4, (1 << 31)
	bne x4, t4, unexpected_irq
	call led_green
	call delay
	j irq_source_check_done
unexpected_irq:
	call led_red
	call delay
irq_source_check_done:
	picorv32_retirq_insn() // Return from interrupt


init:
	li t0, 0x3fffffff // IRQ31 & IRQ30 mask
	picorv32_maskirq_insn(zero, t0) // Enable IRQs

irq_trigger_loop:
	li t0, 0xe0000000 // IRQ31 trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.

	li t0, 0xe1000000 // IRQ30 trigger address
	sw zero, 0(t0) // Raise IRQ by writing to interrupt trigger address.
	               // Writing any data triggers an interrupt.
	j irq_trigger_loop

led_red:
	li t2, 0xff000024
	li t3, (1 << 2)
	sw t3, 0(t2)
	ret

led_green:
	li t2, 0xff000024
	li t3, (1 << 1)
	sw t3, 0(t2)
	ret

led_blue:
	li t2, 0xff000024
	li t3, (1 << 0)
	sw t3, 0(t2)
	ret

delay:
	li t5, 0x100000
delay_dec:
	addi t5, t5, -1
	bne t5, zero, delay_dec
	ret

	.align 4 // Padding to please makehex.py which requires even 4-byte file
	         // sizes.

