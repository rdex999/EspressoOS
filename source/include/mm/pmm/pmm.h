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

#define PMM_BLOCK_SIZE 4096
#define PMM_BITMAP_ADDRESS KERNEL_END

/* Each bit specifies an available(0) or unavailable(1) memory block of PMM_BLOCK_SIZE bytes. */
#define PMM_BITMAP ((pmm_bitmap_t)PMM_BITMAP_ADDRESS)
#define PMM_BITMAP_BITS_IN_ENTRY (sizeof(PMM_BITMAP[0]) * 8)					/* The number of bits in a bitmap entry. */
#define PMM_BITMAP_BYTES_IN_ENTRY (sizeof(PMM_BITMAP[0]))						/* The number of bytes in a bitmap entry. */
#define PMM_BITMAP_SIZE (pmm_total_blocks / PMM_BITMAP_BITS_IN_ENTRY)			/* The size of the bitmap in bytes. */

/* Dont cancle me for using globals, there isnt realy a better way for doing this */
extern size_t pmm_total_blocks;
extern size_t pmm_free_blocks;
extern size_t pmm_used_blocks;

/* NOTE: usualy, "block" referse to a bit in the bitmap */

/* Initializes the physical memory manager */
void pmm_init(multiboot_tag_mmap_t* mmap);

/* Marks a memory block as used in the bitmap */
void pmm_bitmap_alloc(size_t block);

/* Marks a memory block as free in the bitmap */
void pmm_bitmap_free(size_t block);

/* Check if a memory block is free */
bool pmm_bitmap_is_free(size_t block);

/* Mark <count> blocks as used in the bitmap */
void pmm_bitmap_alloc_blocks(size_t start_block, size_t count);

/* Mark <count> blocks as free in the bitmap */
void pmm_bitmap_free_blocks(size_t start_block, size_t count);

/* Converts a physical address to its index in the bitmap (block) */
size_t pmm_bitmap_addr_to_block(uint64_t address);

/* Converts a bitmap index (block) into its physical address */
uint64_t pmm_bitmap_block_to_addr(size_t block);