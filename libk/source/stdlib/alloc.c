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
		size_t pages = DIV_ROUND_UP(size + sizeof(block_meta_t), VMM_PAGE_SIZE);
		s_first_block = (block_meta_t*)vmm_alloc_pages(VMM_PAGE_P | VMM_PAGE_RW, pages);
		if((virt_addr_t)s_first_block == (virt_addr_t)-1)
			return NULL;

		s_first_block->prev = NULL;
		s_first_block->free = false;
		s_first_block->size = size;
		if(size < pages * VMM_PAGE_SIZE)
		{
			block_meta_t* next = BLOCK_NEXT_IN_PAGE(s_first_block);
			s_first_block->next = next;
			next->prev = s_first_block;
			*next = {
				.next = NULL,
				.prev = s_first_block,
				.free = true,
				.size = (uint64_t)s_first_block + pages * VMM_PAGE_SIZE - (uint64_t)BLOCK_START(next)
			};
		}
		else
			s_first_block->next = NULL;
		
		return BLOCK_START(s_first_block);
	}
	
	block_meta_t* block = s_first_block;
	block_meta_t* last_block = block;
	while(block)
	{
		if(block->free && block->size >= size)
		{
			alloc_alloc_block(block, size);
			return BLOCK_START(block);
		}

		last_block = block;
		block = block->next;
	}

	size_t chunk_size = ALIGN_UP(size, VMM_PAGE_SIZE);
	size_t pages = chunk_size / VMM_PAGE_SIZE;
	block_meta_t* new_block = (block_meta_t*)vmm_alloc_pages(VMM_PAGE_P | VMM_PAGE_RW, pages);
	if((virt_addr_t)new_block == (virt_addr_t)-1)
		return NULL;

	last_block->next = new_block;
	*new_block = {
		.next = NULL,
		.prev = last_block,
		.free = true,
		.size = chunk_size - sizeof(block_meta_t)
	};

	alloc_alloc_block(new_block, size);

	return BLOCK_START(new_block);
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
		last = block;

		if(!IS_NEXT_IN_PAGE(block))
			break;
		
		block = block->next;
	}

	block_meta_t* new_block = BLOCK_NEXT_IN_PAGE(after);
	after->next = new_block;
	*new_block = {
		.next = block,
		.prev = after,
		.free = true,
		.size = (uint64_t)BLOCK_END(last) - (uint64_t)BLOCK_START(new_block)
	};

	if(block != NULL)
		block->prev = new_block;

	return new_block;
}

void alloc_alloc_block(block_meta_t* block, size_t size)
{
	if(block->size - size < sizeof(block_meta_t) || (block->next != NULL && !IS_NEXT_IN_PAGE(block)))
	{
		block->free = false;
		return;
	}

	if(block->next == NULL)
	{
		/* 
		 * If there is no next block, then just resize <block> and insert a new block
		 * describing the free region until the end of the page. 
		 */
		size_t old_size = block->size;
		block->free = false;
		block->size = size;

		block_meta_t* new_next = BLOCK_NEXT_IN_PAGE(block);
		*new_next = {
			.next = NULL,
			.prev = block,
			.free = true,
			.size = old_size - (size + sizeof(block_meta_t))
		};
		block->next = new_next;
		
		return;
	}

	/* Resize <block>, mark it as allocated */
	block->size = size;
	block->free = false;

	/* Insert a free block right after <block>. */	
	block_meta_t* old_next = block->next;
	block_meta_t* new_next = BLOCK_NEXT_IN_PAGE(block);
	
	*new_next = {
		.next = old_next,
		.prev = block,
		.free = true,
		.size = (uint64_t)old_next - (uint64_t)BLOCK_START(new_next)
	};

	block->next = new_next;
	old_next->prev = new_next;

	/* Because we just inserted a free block, there might be a free block after that, so in case there is merge them. */
	alloc_merge_free(block);
}