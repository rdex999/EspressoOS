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
size_t g_pmm_total_blocks	= -1;
size_t g_pmm_free_blocks 	= -1;
size_t g_pmm_used_blocks 	= -1;

void pmm_init(multiboot_tag_mmap_t*)
{
	memset(PMM_BITMAP, 0, 1024);
}

uint64_t pmm_alloc()
{
	size_t block = pmm_bitmap_find_free();
	if(block == -1)
		return -1;

	pmm_bitmap_alloc(block);
	return pmm_bitmap_block_to_addr(block);
}

void pmm_free(uint64_t address)
{
	pmm_free_blocks(address, 1);
}

void pmm_free_blocks(uint64_t address, size_t count)
{
	size_t block = pmm_bitmap_addr_to_block(address);
	pmm_bitmap_free_blocks(block, count);
}

void pmm_alloc_address(uint64_t address, size_t count)
{
	size_t block = pmm_bitmap_addr_to_block(address);
	pmm_bitmap_alloc_blocks(block, count);
}

/* In both of these functions, for some reason, i need to explicitly say the "1" is an unsigned 64 bit integer (llu) */
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

void pmm_bitmap_alloc_blocks(size_t start_block, size_t count)
{
	/* 
	 * This function has 4 steps
	 * 	- 1 => Check if <start_block> and <count> both start and end at an entry index, if so, memset the memory to 1 and return.
	 *	- 2 => Mark the bits in the first entry, so <start_block> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, mark the bits left in the entry)
	 *	- 3 => Check if we can use memset to set full bitmap entries.
	 *	- 4 => Mark the unaligned bits in the last entry, as they were not allocated in the memset.
	 */

	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;	/* The entry index for <start_block> in the bitmap */
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;			/* The bit offset in <entry_index> for the first bit to allocate */

	/* If <start_block> and <count> both start and end at an entry index, we can just memset the memory to 1 */
	size_t full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can set using memset. */
	if(bit_index == 0 && full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, count);	/* The amount of bits to set in the first entry */
		PMM_BITMAP[entry_index] |= ((1llu << first_entry_blocks) - 1) << bit_index;		/* Set <first_entry_blocks> bits, then shift them to their index in the entry */

		++entry_index;											/* Finished the first entry, so go to the next one */
		count -= first_entry_blocks;							/* Decrease the amount of blocks left to allocate */
		start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */
	}

	/* 
	 * Here, we know the <start_block> will be aligned to an entry in the bitmap. 
	 * Also, if count is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to set full entries (uint64_t), do it. */
	full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;		/* The amount of full entries we can set using memset. */
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Allocate the blocks that were not allocated in the memset */
	PMM_BITMAP[entry_index] |= (1llu << count) - 1llu;			/* Set the remaining bits in the last entry */
}

void pmm_bitmap_free_blocks(size_t start_block, size_t count)
{
	/* 
	 * This function has 4 steps (similar to pmm_bitmap_alloc_blocks)
	 * 	- 1 => Check if <start_block> and <count> both start and end at an entry index, if so, memset the memory to 0 and return.
	 *	- 2 => Clear the bits in the first entry, so <start_block> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, clear the bits left in the entry)
	 *	- 3 => Check if we can use memset to clear full bitmap entries.
	 *	- 4 => Clear the unaligned bits in the last entry, as they were not cleared in the memset.
	 */

	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;	/* The entry index for <start_block> in the bitmap */
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;			/* The bit offset in <entry_index> for the first bit to free */

	/* If <start_block> and <count> both start and end at an entry index, we can just memset the memory to 0 */
	size_t full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can clear using memset. */
	if(bit_index == 0 && full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, count);	/* The amount of bits to clear in the first entry */
		PMM_BITMAP[entry_index] &= ~(((1llu << first_entry_blocks) - 1) << bit_index);	/* Clear <first_entry_blocks> bits, then shift them to their index in the entry */

		++entry_index;											/* Finished the first entry, so go to the next one */
		count -= first_entry_blocks;							/* Decrease the amount of blocks left to free */
		start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */
	}

	/* 
	 * Here, we know the <start_block> will be aligned to an entry in the bitmap. 
	 * Also, if count is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to clear full entries (uint64_t), do it. */
	full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can clear using memset. */
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Free the blocks that were not freed in the memset */
	PMM_BITMAP[entry_index] &= ~((1llu << count) - 1llu);		/* Clear the remaining bits in the last entry */
}

size_t pmm_bitmap_addr_to_block(uint64_t address)
{
	return address / PMM_BLOCK_SIZE;
}

uint64_t pmm_bitmap_block_to_addr(size_t block)
{
	return block * PMM_BLOCK_SIZE;
}

size_t pmm_bitmap_find_free(size_t start_block)
{
	for(size_t i = start_block / PMM_BITMAP_BITS_IN_ENTRY; i < PMM_BITMAP_LENGTH; ++i)
	{
		if (PMM_BITMAP[i] != -1)
		{
			/* 
			 * The __builtin_ffsll function performs the BSF (byte scan forward) instruction, 
			 * which finds the index of the first set bit. However, __builtin_ffsll increases the result of BSF by 1
			 * so we need to subtract 1 to get the bit index.
			 * Here we find the first bit that is set in the result of the bitwise NOT operation on the bitmap entry.
			 * (Which is, the first cleared bit in the bitmap entry)
			 */
			size_t bit_offset = __builtin_ffsll(~PMM_BITMAP[i]) - 1;	/* ffsll counts from 1, so subtract 1 */
			return i * PMM_BITMAP_BITS_IN_ENTRY + bit_offset;
		}
	}
	return -1;
}

size_t pmm_bitmap_find_free()
{
	return pmm_bitmap_find_free(0);
}