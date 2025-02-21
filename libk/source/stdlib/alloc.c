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
		size_t chunk_size = ALIGN_UP(size, VMM_PAGE_SIZE);
		size_t pages = chunk_size / VMM_PAGE_SIZE;

		s_first_block = (block_meta_t*)vmm_alloc_pages(VMM_PAGE_P | VMM_PAGE_RW, pages);
		if((virt_addr_t)s_first_block == (virt_addr_t)-1)
			return NULL;

		*s_first_block = {
			.next = NULL,
			.prev = NULL,
			.free = true,
			.size = chunk_size - sizeof(block_meta_t)
		};
		alloc_alloc_block(s_first_block, size);
		
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

void free(void* ptr)
{
	/* 
	 * So, travel the list backwards, and find the first free block (on the same chunk/pages), so we can merge them.
	 * After merging, check if the free block is aligned to 4KiB and its size is bigger than 4KiB, if so, free the page/s.
	 * If its not aligned, check if it covers an aligned page 
	 * (meaning its size is big enough to cover the rest of the current page, on the whole next page/s), 
	 * If it does cover other pages, free them.
	 */

	if(ptr == NULL)
		return;

	block_meta_t* block = (block_meta_t*)((uint64_t)ptr - sizeof(block_meta_t));
	block->free = true;

	block_meta_t* first_free = block;
	while(first_free->prev != NULL && first_free->prev->free && IS_NEXT_IN_PAGE(first_free->prev))
		first_free = first_free->prev;

	alloc_merge_free(first_free);

	uint64_t until_next_page = ALIGN_UP((uint64_t)first_free, VMM_PAGE_SIZE) - (uint64_t)first_free;
	if(IS_ALIGNED((uint64_t)first_free, VMM_PAGE_SIZE) && first_free->size + sizeof(block_meta_t) >= VMM_PAGE_SIZE)
	{
		block_meta_t* prev = first_free->prev;
		if(prev != NULL)
			prev->next = first_free->next;
		else
			s_first_block = first_free->next;

		size_t pages = (first_free->size + sizeof(block_meta_t)) / VMM_PAGE_SIZE;
		vmm_free_pages((virt_addr_t)first_free, pages);
	} 
	else if(first_free->size + sizeof(block_meta_t) > VMM_PAGE_SIZE + until_next_page)
	{
		void* next_page = (void*)((uint64_t)first_free + until_next_page);
		size_t pages = ((first_free->size + sizeof(block_meta_t)) - until_next_page) / VMM_PAGE_SIZE;
		first_free->size = until_next_page - sizeof(block_meta_t);
		vmm_free_pages((virt_addr_t)next_page, pages);
	}
}

void alloc_merge_free(block_meta_t* block)
{
	if(block == NULL || (block->next != NULL && !IS_NEXT_IN_PAGE(block)) || !block->free)
		return;
	
	block_meta_t* current = block;
	block_meta_t* last_free = block;
	while(current != NULL && current->free)
	{
		last_free = current;

		if(current->next != NULL && !IS_NEXT_IN_PAGE(current))
			break;
		
		current = current->next;
	}

	block->next = current;
	block->size = (uint64_t)BLOCK_END(last_free) - (uint64_t)BLOCK_START(block);
	if(current != NULL)
		current->prev = block;
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
	alloc_merge_free(block->next);
}