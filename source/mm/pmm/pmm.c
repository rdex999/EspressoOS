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
	memset(PMM_BITMAP, 0, 1024);
}

/* In both of these functions, for some reason, i need to explicitly say the "1" is an unsigned 64 bit integer */
void pmm_bitmap_alloc(size_t block)
{
	PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] |= 1llu << (block % PMM_BITMAP_BITS_IN_ENTRY);
}

void pmm_bitmap_free(size_t block)
{
	PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] &= ~(1llu << (block % PMM_BITMAP_BITS_IN_ENTRY));
}

bool pmm_bitmap_is_free(size_t block)
{
	return (PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] & (1llu << (block % PMM_BITMAP_BITS_IN_ENTRY))) == 0;
}

/* TODO: Optimize the for loops in pmm_bitmap_alloc_blocks and pmm_bitmap_free_blocks */

void pmm_bitmap_alloc_blocks(size_t start_block, size_t count)
{
	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;

	/* If <start_block> and <count> both start and end at an index, we can just memset the memory to 1 */
	if(bit_index == 0 && count % PMM_BITMAP_BITS_IN_ENTRY == 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, (count / PMM_BITMAP_BITS_IN_ENTRY) * PMM_BITMAP_BYTES_IN_ENTRY);
		return;
	}

	/* Finish the first entry, then if there are more check if using memset is possible */
	size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, count);
	for(size_t i = 0; i < first_entry_blocks; i++)
		pmm_bitmap_alloc(start_block + i);

	++entry_index;											/* Finished the first entry, so go to the next one */
	count -= first_entry_blocks;							/* Decrease the amount of blocks left to allocate */
	start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */

	/* <count> Can only be greater or equal to zero. If there are no more blocks to allocate, return. */
	if (count == 0)
		return;

	/* If we can use memset to set full entries (uint64_t), do it. */
	size_t full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		// entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Allocate the blocks that were not allocated in the memset */
	for(size_t i = 0; i < count; i++)
		pmm_bitmap_alloc(start_block + i);
}

void pmm_bitmap_free_blocks(size_t start_block, size_t count)
{
	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;

	/* If <start_block> and <count> both start and end at an entry index, we can just memset the memory to 0 */
	if(bit_index == 0 && count % PMM_BITMAP_BITS_IN_ENTRY == 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, (count / PMM_BITMAP_BITS_IN_ENTRY) * PMM_BITMAP_BYTES_IN_ENTRY);
		return;
	}

	/* Finish the first entry, then if there are more check if using memset is possible */
	size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, count);
	for(size_t i = 0; i < first_entry_blocks; i++)
		pmm_bitmap_free(start_block + i);

	++entry_index;											/* Finished the first entry, so go to the next one */
	count -= first_entry_blocks;							/* Decrease the amount of blocks left to free */
	start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */

	/* <count> Can only be greater or equal to zero. If there are no more blocks to free, return. */
	if (count == 0)
		return;

	/* If we can use memset to set full entries (uint64_t), do it. */
	size_t full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		// entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Free the blocks that were not freed in the memset */
	for(size_t i = 0; i < count; i++)
		pmm_bitmap_free(start_block + i);
}

size_t pmm_bitmap_addr_to_block(uint64_t address)
{
	return address / PMM_BLOCK_SIZE;
}

uint64_t pmm_bitmap_block_to_addr(size_t block)
{
	return block * PMM_BLOCK_SIZE;
}