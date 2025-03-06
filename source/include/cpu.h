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
#include "idt/idt.h"

inline uint64_t read_cr3()
{
	uint64_t res;
	asm volatile("mov %%cr3, %0"
		: "=r"(res)
		:
	);
	return res;
}

inline void write_cr3(uint64_t value)
{
	asm volatile("mov %0, %%cr3"
		:
		: "r"(value)
		: "memory"
	);

}

inline void tlb_native_flush_page(void* virtual_address)
{
	asm volatile("invlpg (%0)"
		:
		: "r"(virtual_address)
		: "memory"
	);
}

/* 
 * For me forgeting everything in the future, the "N" constraint means that if the value can fit in one byte, 
 * it will be passed as a number, otherwise it will be put into a register and passed with the register.
 */
inline void outl(uint16_t port, uint32_t value)
{
	asm volatile("outl %0, %1"
		:
		: "a"(value), "Nd"(port)
	);
}

inline void outw(uint16_t port, uint16_t value)
{
	asm volatile("outw %0, %1"
		:
		: "a"(value), "Nd"(port)
	);
}

inline void outb(uint16_t port, uint8_t value)
{
	asm volatile("outb %0, %1"
		:
		: "a"(value), "Nd"(port)
	);
}

inline uint32_t inl(uint16_t port)
{
	uint32_t res;
	asm volatile("inl %1, %0"
		: "=a"(res)
		: "Nd"(port)
	);
	return res;
}

inline uint16_t inw(uint16_t port)
{
	uint16_t res;
	asm volatile("inw %1, %0"
		: "=a"(res)
		: "Nd"(port)
	);
	return res;
}

inline uint8_t inb(uint16_t port)
{
	uint8_t res;
	asm volatile("inb %1, %0"
		: "=a"(res)
		: "Nd"(port)
	);
	return res;
}

inline void load_idt(const idt_descriptor_t* idt_descriptor)
{
	asm volatile("lidt %0"
		: 
		: "m"(idt_descriptor)
	);
}

inline void read_idtr(idt_descriptor_t* idt_descriptor)
{
	asm volatile("sidt %0"
		: "=m"(*idt_descriptor)
	);
}