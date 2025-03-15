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

/* 
 * For more info about CPUID, its codes and return values see intel documentation at: 
 * https://www.intel.com/content/www/us/en/content-details/843860/intel-architecture-instruction-set-extensions-programming-reference.html?wapkw=Intel%20Architecture%20Instruction%20Set%20Extensions%20Programming%20Reference
 * Page 18 in the PDF version. 
 */
#define CPUID_CODE_GET_FEATURES 				1

#define CPUID_FEATURE_EDX_APIC 					(1 << 9)
#define CPUID_FEATURE_EBX_INIT_APIC_ID(ebx)		(((ebx) >> 24) & 0xFF)
#define CPUID_FEATURE_ECX_POPCNT       			(1 << 23)

#define MSR_IA32_APIC_BASE						0x1B

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
		: "m"(*idt_descriptor)
	);
}

inline void read_idtr(idt_descriptor_t* idt_descriptor)
{
	asm volatile("sidt %0"
		: "=m"(*idt_descriptor)
	);
}

/* 
 * Perform the CPUID instruction with EAX=<code>. 
 * Note: <eax>, <ebx>, <ecx>, <edx> must point to a valid memory address. 
 */
inline void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
	asm volatile("cpuid"
		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(code)
	);
}

inline uint64_t cpu_read_msr(uint32_t msr)
{
	uint32_t low, high;
	asm volatile("rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);

	return (uint64_t)low | ((uint64_t)high << 32);
}

inline void cpu_write_msr(uint32_t msr, uint64_t value)
{
	asm volatile("wrmsr"
		:
		: "c"(msr), "a"((uint32_t)value), "d"((uint32_t)(value >> 32))
	);
}