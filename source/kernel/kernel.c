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

#include "kernel/kernel.h"

#include "string.h"

#define VIDEO ((uint32_t*)0xA0000)

#ifdef __cplusplus
	extern "C"
#endif
void kernel_main(multiboot_info_t* mbd)
{
	VIDEO[0] = 0x00FF0000;	/* RED */
	VIDEO[1] = 0x0000FF00;	/* GREEN */
	VIDEO[2] = 0x000000FF;	/* BLUE */

	multiboot_tag_mmap_t* tag = (multiboot_tag_mmap_t*)mbd->find_tag(MULTIBOOT_TAG_TYPE_MMAP);
	multiboot_mmap_entry_t* m0 = tag->index(0);
	multiboot_mmap_entry_t* m1 = tag->index(1);
	multiboot_mmap_entry_t* m2 = tag->index(2);

	memset(&VIDEO[3], 0xFF, 1024);

	while(1)
	{
		asm("cli");
		asm("hlt");
	}
}