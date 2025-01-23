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

#include <multiboot.h>
#include <stdint.h>
#include <stddef.h>

#define PMM_BLOCK_SIZE 4096
#define KERNEL_END (void*)&_kernel_end
#define PMM_BITMAP_ADDRESS KERNEL_END

/* Each bit specifies an available(0) or unavailable(1) memory block of PMM_BLOCK_SIZE bytes. */
typedef struct pmm_bitmap
{
	uint64_t bitmap[0];
} pmm_bitmap_t;

extern uint64_t _kernel_end;
extern pmm_bitmap_t* pmm_bitmap;	/* A pointer to the bitmap, defined in pmm.c */

void pmm_init(multiboot_tag_mmap_t* mmap);