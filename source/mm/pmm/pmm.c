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
size_t pmm_total_blocks	= -1;
size_t pmm_free_blocks 	= -1;
size_t pmm_used_blocks 	= -1;

void pmm_init(multiboot_tag_mmap_t*)
{
}