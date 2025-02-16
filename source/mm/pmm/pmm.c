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

#include "mm/pmm/pmm.h"

/* A value of -1 indicates these are uninitialized. pmm_init() Should initialize them */
size_t g_pmm_total_blocks	= -1;
size_t g_pmm_memory_blocks	= -1;

bitmap g_pmm_bitmap;

void pmm_init(multiboot_tag_mmap_t* mmap)
{
	/* 
	 * In the provided memory map, not all addresses are actual ram. About 11GiB will be reserved for devices (graphics card, etc).
	 * So, the memory the bitmap will look after will be 
	 * from 0, to the highest available memory address, plus 1. (Amount of bytes)
	 * <highest_available_memory> Will represent the highest address of available memory plus one.
	 * <total_available_ram> Will represent the total amount of available ram in bytes.
	 */

	size_t total_mmap_memory = 0;
	size_t total_available_ram = 0;
	phys_addr_t highest_available_memory = 0;
	for(size_t i = 0; i < mmap->entries_length(); ++i)
	{
		multiboot_mmap_entry_t* entry = mmap->index(i);
		total_mmap_memory += entry->len;

		if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
		{
			total_available_ram += entry->len;

			if (entry->addr + entry->len > highest_available_memory)
				highest_available_memory = entry->addr + entry->len;
		}
	}
	g_pmm_total_blocks = total_mmap_memory / PMM_BLOCK_SIZE;
	g_pmm_memory_blocks = highest_available_memory / PMM_BLOCK_SIZE;

	/* Create the bitmap */
	new(&g_pmm_bitmap) bitmap(PMM_BITMAP_ADDRESS, PMM_BITMAP_SIZE);

	/* Mark unavailable blocks as used */
	for(size_t i = 0; i < mmap->entries_length(); ++i)
	{
		multiboot_mmap_entry_t* entry = mmap->index(i);
		if(entry->type != MULTIBOOT_MEMORY_AVAILABLE)
		{
			phys_addr_t aligned_addr = ALIGN_DOWN(entry->addr, PMM_BLOCK_SIZE);
			size_t real_length = entry->len + (entry->addr - aligned_addr);
			size_t blocks = DIV_ROUND_UP(real_length, PMM_BLOCK_SIZE);
			pmm_alloc_address(aligned_addr, blocks);
		}
	}
}

phys_addr_t pmm_alloc()
{
	size_t block = g_pmm_bitmap.allocate();
	return pmm_block_to_addr(block);
}

void pmm_free(phys_addr_t address)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	pmm_free_blocks(aligned_address, 1);
}

void pmm_free_blocks(phys_addr_t address, size_t count)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	size_t block = pmm_addr_to_block(aligned_address);
	g_pmm_bitmap.free(block, count);
}

void pmm_alloc_address(phys_addr_t address, size_t count)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	size_t block = pmm_addr_to_block(aligned_address);
	g_pmm_bitmap.set(block, count);
}

bool pmm_is_free(phys_addr_t address)
{
	size_t aligned = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	size_t block = pmm_addr_to_block(aligned);
	return g_pmm_bitmap.is_clear(block);
}

size_t pmm_addr_to_block(phys_addr_t address)
{
	return address / PMM_BLOCK_SIZE;
}

phys_addr_t pmm_block_to_addr(size_t block)
{
	return block * PMM_BLOCK_SIZE;
}