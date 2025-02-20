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

#include "stdlib/alloc.h"

static block_meta_t* s_first_block = NULL;

void* malloc(size_t size)
{
	if (size == (size_t)0)
		return NULL;
	
	if(s_first_block == NULL)
	{
		size_t pages = DIV_ROUND_UP(size, VMM_PAGE_SIZE);
		s_first_block = (block_meta_t*)vmm_alloc_pages(VMM_PAGE_P | VMM_PAGE_RW, pages);
		if((virt_addr_t)s_first_block == (virt_addr_t)-1)
			return NULL;

		s_first_block->prev = NULL;
		s_first_block->free = false;
		s_first_block->size = size;
		if(size % VMM_PAGE_SIZE != 0)
		{
			block_meta_t* next = BLOCK_NEXT_IN_PAGE(s_first_block);
			s_first_block->next = next;
			next->prev = s_first_block;	
		}
		else
			s_first_block->next = NULL;
	}
	
	// block_meta_t* block = s_first_block;
	// block_meta_t* last_block = block;
	// while(block)
	// {
		
	// }
	return NULL;
} 

block_meta_t* alloc_merge_free(block_meta_t* after)
{
	block_meta_t* block = after->next;
	if(block == NULL)
		return NULL;

	if(!IS_NEXT_IN_PAGE(after))
		return NULL;

	block_meta_t* last = block;
	while(block && block->free)
	{
		if(!IS_NEXT_IN_PAGE(block))
			break;
		
		last = block;
		block = block->next;
	}

	block_meta_t* new_block = BLOCK_NEXT_IN_PAGE(after);
	after->next = new_block;
	new_block->prev = after;
	new_block->free = true;
	new_block->size = ((uint64_t)last - (uint64_t)BLOCK_START(new_block));
	last->prev = new_block;

	return new_block;
}