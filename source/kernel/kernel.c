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
#include <string.h>
#include <stdlib.h>
#include "mm/pmm/pmm.h"
#include "mm/vmm/vmm.h"

#define VIDEO ((uint32_t*)0xA0000)

#ifdef __cplusplus
	extern "C"
#endif
void kernel_main(multiboot_info_t* mbd)
{
	multiboot_tag_mmap_t* mmap = (multiboot_tag_mmap_t*)mbd->find_tag(MULTIBOOT_TAG_TYPE_MMAP);
	if(mmap == NULL)	/* Always do null checks people, you dont want a damn headache. */
		while(true) { asm volatile("cli"); asm volatile("hlt"); }

	pmm_init(mmap);
	vmm_init();

	/* ITS FOR TESTING */
	uint8_t* b1 = (uint8_t*)malloc(40);
	uint8_t* b2 = (uint8_t*)malloc(90);
	uint8_t* b3 = (uint8_t*)malloc(20);
	uint8_t* b4 = (uint8_t*)malloc(3700);
	uint64_t* b5 = (uint64_t*)malloc(5000);
	uint64_t* b6 = (uint64_t*)malloc(80);

	free(b5);
	free(b6);
	free(b4);
	free(b3);
	free(b2);
	free(b1);

	while(true) { asm volatile("cli"); asm volatile("hlt"); }
} 