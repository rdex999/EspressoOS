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