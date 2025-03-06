/* 
 * This file is part of the EspressoOS project (https://github.com/rdex999/EspressoOS.git).
 * Copyright (c) 2025 David Weizman.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#define IDT_GATE_GET_ADDRESS(gate) ((gate)->address0 | ((gate)->address16 << 16) | ((gate)->address32 << 32))
#define IDT_GATE_SET_ADDRESS(gate, address) (( 				\
	(gate)->address0 	= (address) & 0xFFFF;				\
	(gate)->address16 	= ((address) >> 16) & 0xFFFF;		\
	(gate)->address32	= ((address) >> 32) & 0xFFFFFFFF;	\
))

/* See: https://wiki.osdev.org/Interrupt_Descriptor_Table for more information about the structures and stuff */

typedef struct idt_descriptor
{
	uint16_t size;				/* One byte less than the size of the IDT. */
	uint64_t address;			/* The virtual address of the descriptor. */
} __attribute__((packed)) idt_descriptor_t;

/* 
 * <address> Is the virtual address of the ISR. 
 * The 0, 16, 32, ..., means the bits of the address (as its split into three parts)
 * So <address0> of type uint16_t, means bits 0-15 of <address>.
 */
typedef struct idt_gate
{
	uint16_t address0;
	uint16_t segment_selector;		/* Set to the code segment in GDT */
	uint8_t ist;					/* Interrupt stack table, idk */
	uint8_t attributes;				/* Gate type, DPL, and present fields */
	uint16_t address16;
	uint32_t address32;
	uint8_t reserved[8];			/* Must be set to 0 */
} __attribute__((packed)) idt_gate_t;

/* Initialize the IDT. Returns 0 on success, an error code otherwise. */
int idt_init();

/* Set the value of a gate in the IDT. Returns 0 on success, an error code otherwise. */
int idt_set_gate(unsigned int index, const idt_gate_t* gate);

/* Make gate <index> an interrupt gate which points to <isr_address>. Returns 0 on success, an error code otherwise. */
int idt_set_interrupt_gate(unsigned int index, uint64_t isr_address);

/* Make gate <index> a trap gate (exceptions) which points to <isr_address>. Returns 0 on success, an error code otherwise. */
int idt_set_trap_gate(unsigned int index, uint64_t isr_address);

/* Get the address of the IDT. */
idt_gate_t** idt_get_table();