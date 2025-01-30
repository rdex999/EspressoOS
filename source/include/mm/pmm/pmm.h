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

#include <string.h>
#include "multiboot.h"
#include "common.h"
#include <stdint.h>
#include <stddef.h>

typedef uint64_t* pmm_bitmap_t;
typedef uint64_t phys_addr_t;

#define PMM_BLOCK_SIZE 4096

/* Each bit specifies an available(0) or unavailable(1) memory block of PMM_BLOCK_SIZE bytes. */
#define PMM_BITMAP ((pmm_bitmap_t)PMM_BITMAP_ADDRESS)
#define PMM_BITMAP_ADDRESS KERNEL_END
#define PMM_BITMAP_END_ADDRESS ((void*)((uint64_t)PMM_BITMAP_ADDRESS + PMM_BITMAP_SIZE))
#define PMM_BITMAP_BITS_IN_ENTRY (sizeof(PMM_BITMAP[0]) * 8)					/* The number of bits in a bitmap entry. */
#define PMM_BITMAP_BYTES_IN_ENTRY (sizeof(PMM_BITMAP[0]))						/* The number of bytes in a bitmap entry. */
#define PMM_BITMAP_SIZE (g_pmm_total_blocks / 8)								/* The size of the bitmap in bytes. */
#define PMM_BITMAP_LENGTH (PMM_BITMAP_SIZE / PMM_BITMAP_BYTES_IN_ENTRY)			/* The length of the bitmap in entries. */

/* Dont cancle me for using globals, there isnt realy a better way for doing this */
extern size_t g_pmm_total_blocks;		/* The total amount of memory blocks in ram */
extern size_t g_pmm_free_blocks;		/* The amount of blocks that are currently free */
extern size_t g_pmm_used_blocks;		/* The amount of blocks that are currently used */

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

/* Marks a memory block as used in the bitmap */
void pmm_bitmap_alloc(size_t block);

/* Marks a memory block as free in the bitmap */
void pmm_bitmap_free(size_t block);

/* Check if a memory block is free */
bool pmm_bitmap_is_free(size_t block);

/* Check if all <count> blocks are free starting from <start_block>. */
bool pmm_bitmap_is_free_blocks(size_t start_block, size_t count);

/* Mark <count> blocks as used in the bitmap */
void pmm_bitmap_alloc_blocks(size_t start_block, size_t count);

/* Mark <count> blocks as free in the bitmap */
void pmm_bitmap_free_blocks(size_t start_block, size_t count);

/* Converts a physical address to its index in the bitmap (block) */
size_t pmm_bitmap_addr_to_block(phys_addr_t address);

/* Converts a bitmap index (block) into its physical address */
phys_addr_t pmm_bitmap_block_to_addr(size_t block);

/* 
 * Finds the first free block (bit) in the bitmap. Returns its bit index (block index). 
 * Returns -1 if not found.
 */
size_t pmm_bitmap_find_free();

/* 
 * Finds the first free block (bit) in the bitmap starting from <start_block>. Returns its bit index (block index). 
 * Returns -1 if not found.
 */
size_t pmm_bitmap_find_free(size_t start_block);