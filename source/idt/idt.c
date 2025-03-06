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

#include "idt/idt.h"
#include <string.h>
#include "cpu.h"
#include "error.h"
#include "common.h"

__attribute__((aligned(0x10)))
static idt_gate_t s_idt_table[256] = {};		/* initialize to 0 */

extern "C" void isr_exception_page_fault();

int idt_init()
{
	idt_descriptor_t descriptor = {
		.size = sizeof(idt_gate_t) * 256 - 1,
		.address = (uint64_t)&s_idt_table
	};

	idt_set_trap_gate(14, (uint64_t)isr_exception_page_fault);
	
	load_idt(&descriptor);

	return SUCCESS;
}

void idt_set_gate(uint8_t index, const idt_gate_t* gate)
{
	s_idt_table[index] = *gate;
}

void idt_set_interrupt_gate(uint8_t index, uint64_t isr_address)
{
	idt_gate_t gate = {};
	IDT_GATE_SET_ADDRESS(&gate, isr_address);
	IDT_GATE_SET_IST(&gate, 0);
	gate.attributes = IDT_ATTR_GATE_TYPE_INTERRUPT | IDT_ATTR_PRESENT;
	gate.segment_selector = GDT_CODE_SELECTOR;
	gate.reserved = 0;

	idt_set_gate(index, &gate);
}

void idt_set_trap_gate(uint8_t index, uint64_t isr_address)
{
	idt_gate_t gate = {};
	IDT_GATE_SET_ADDRESS(&gate, isr_address);
	IDT_GATE_SET_IST(&gate, 0);
	gate.attributes = IDT_ATTR_GATE_TYPE_TRAP | IDT_ATTR_PRESENT;
	gate.segment_selector = GDT_CODE_SELECTOR;
	gate.reserved = 0;

	idt_set_gate(index, &gate);
}

extern "C" void interrupt_page_fault(uint32_t error)
{
	return;
}