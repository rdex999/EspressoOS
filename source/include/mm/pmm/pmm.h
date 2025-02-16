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
#include <new>
#include <string.h>
#include "multiboot.h"
#include "common.h"
#include "ds/bitmap.h"

typedef uint64_t phys_addr_t;

#define PMM_BLOCK_SIZE 4096

/* Each bit specifies an available(0) or unavailable(1) memory block of PMM_BLOCK_SIZE bytes. */
#define PMM_BITMAP_ADDRESS 			KERNEL_END
#define PMM_BITMAP_END_ADDRESS 		((void*)((uint64_t)PMM_BITMAP_ADDRESS + PMM_BITMAP_SIZE))
#define PMM_BITMAP_SIZE 			(g_pmm_memory_blocks / 8)						/* The size of the bitmap in bytes. */

/* Dont cancle me for using globals, there isnt realy a better way for doing this */
extern size_t g_pmm_total_blocks;		/* The total amount of memory from the memory map, including memory-mapped devices. */
extern size_t g_pmm_memory_blocks;		/* The total amount of memory blocks in ram */

extern bitmap g_pmm_bitmap;				/* The bitmap of physical blocks. allocated (1) or free (0) */

/* NOTE: usualy, "block" referse to a bit in the bitmap */

/* Initializes the physical memory manager */
void pmm_init(multiboot_tag_mmap_t* mmap);

/* Allocates a single block of memory, returns its physical address. Returns -1 on failure. */
phys_addr_t pmm_alloc();

/* Frees a single block of memory. <address> will be aligned down to <PMM_BLOCK_SIZE>. */
void pmm_free(phys_addr_t address);

/* Frees <count> blocks of memory. <address> will be aligned down to <PMM_BLOCK_SIZE>.*/
void pmm_free_blocks(phys_addr_t address, size_t count);

/* Marks <count> blocks of memory at <address> as allocated. <address> will be aligned down to <PMM_BLOCK_SIZE>. */
void pmm_alloc_address(phys_addr_t address, size_t count);

/* Check if the block at <address> is free. Returns true if its free, false otherwise. <address> will be aligned down to <PMM_BLOCK_SIZE>. */
bool pmm_is_free(phys_addr_t address);

/* Converts a physical address to its index in the bitmap (block) */
size_t pmm_addr_to_block(phys_addr_t address);

/* Converts a bitmap index (block) into its physical address */
phys_addr_t pmm_block_to_addr(size_t block);
