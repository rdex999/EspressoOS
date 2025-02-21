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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "mm/vmm/vmm.h"
#include "common.h"

#define BLOCK_NEXT_IN_PAGE(block) 	((block_meta_t*)((uint64_t)(block) + (block)->size + sizeof(block_meta_t)))
#define IS_NEXT_IN_PAGE(block) 		(BLOCK_NEXT_IN_PAGE(block) == (block)->next)
#define BLOCK_START(block)			((block_meta_t*)(block) + 1)
#define BLOCK_END(block)			((uint64_t)BLOCK_START(block) + (block)->size)

typedef struct block_meta
{
	struct block_meta* next;
	struct block_meta* prev;
	bool free;
	size_t size;				/* The size of the memory block, not including this structure. */
} block_meta_t;

/* Merge free blocks starting from <after>. */
void alloc_merge_free(block_meta_t* block);

/* Allocate <block>, shrink it to <size> and merge free regions. */
void alloc_alloc_block(block_meta_t* block, size_t size);