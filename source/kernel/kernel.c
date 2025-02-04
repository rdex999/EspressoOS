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
	{
		while(1) 
		{ 
			asm volatile("cli"); 
			asm volatile("hlt");
		}
	}

	pmm_init(mmap);

	virt_addr_t virt = (3llu << 30llu) + 20;

	/* Will cause a page-fault */
	// *(int*)virt = 420;
	// int value = *(int*)virt;

	/* Map virtual address (3<<30) (page directory entry number 3) to 2GiB address. */
	int res = vmm_map_virtual_page(virt, VMM_PAGE_P | VMM_PAGE_RW);
	*(int*)virt = 420;
	int value = *(int*)virt;

	while(1)
	{
		asm volatile("cli");
		asm volatile("hlt");
	}
} 