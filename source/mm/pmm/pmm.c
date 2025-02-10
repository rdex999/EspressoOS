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
size_t g_pmm_mmap_blocks	= -1;
size_t g_pmm_total_blocks	= -1;
size_t g_pmm_free_blocks 	= -1;
size_t g_pmm_used_blocks 	= -1;

void pmm_init(multiboot_tag_mmap_t* mmap)
{
	/* 
	 * In the provided memory map, not all addresses are actual ram. About 11GiB will be reserved for devices (graphics card, etc).
	 * So, the memory the bitmap will look after will be 
	 * from 0, to the highest available memory address, plus 1. (Amount of bytes)
	 * <highest_available_memory> Will represent the highest address of available memory plus one.
	 * <total_available_ram> Will represent the total amount of available ram in bytes.
	 */

	size_t total_mmap_memory = 0;
	size_t total_available_ram = 0;
	phys_addr_t highest_available_memory = 0;
	for(size_t i = 0; i < mmap->entries_length(); ++i)
	{
		multiboot_mmap_entry_t* entry = mmap->index(i);
		total_mmap_memory += entry->len;

		if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
		{
			total_available_ram += entry->len;

			if (entry->addr + entry->len > highest_available_memory)
				highest_available_memory = entry->addr + entry->len;
		}
	}
	g_pmm_mmap_blocks = total_mmap_memory / PMM_BLOCK_SIZE;
	g_pmm_total_blocks = highest_available_memory / PMM_BLOCK_SIZE;
	g_pmm_free_blocks = total_available_ram / PMM_BLOCK_SIZE;
	g_pmm_used_blocks = 0;

	/* Initialize the bitmap to all zeros */
	memset(PMM_BITMAP, 0, PMM_BITMAP_SIZE);

	/* Mark unavailable blocks as used */
	for(size_t i = 0; i < mmap->entries_length(); ++i)
	{
		multiboot_mmap_entry_t* entry = mmap->index(i);
		if(entry->type != MULTIBOOT_MEMORY_AVAILABLE)
		{
			phys_addr_t aligned_addr = ALIGN_DOWN(entry->addr, PMM_BLOCK_SIZE);
			size_t real_length = entry->len + (entry->addr - aligned_addr);
			size_t blocks = DIV_ROUND_UP(real_length, PMM_BLOCK_SIZE);
			pmm_alloc_address(aligned_addr, blocks);
		}
	}
}

phys_addr_t pmm_alloc()
{
	size_t block = pmm_bitmap_find_free();
	if(block == -1llu)
		return -1llu;

	pmm_bitmap_alloc(block);
	return pmm_bitmap_block_to_addr(block);
}

void pmm_free(phys_addr_t address)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	pmm_free_blocks(aligned_address, 1);
}

void pmm_free_blocks(phys_addr_t address, size_t count)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	size_t block = pmm_bitmap_addr_to_block(aligned_address);
	pmm_bitmap_free_blocks(block, count);
}

void pmm_alloc_address(phys_addr_t address, size_t count)
{
	phys_addr_t aligned_address = ALIGN_DOWN(address, PMM_BLOCK_SIZE);
	size_t block = pmm_bitmap_addr_to_block(aligned_address);
	pmm_bitmap_alloc_blocks(block, count);
}

bool pmm_is_free(phys_addr_t address)
{
	size_t block = pmm_bitmap_addr_to_block(ALIGN_DOWN(address, PMM_BLOCK_SIZE));
	return pmm_bitmap_is_free(block);
}

/* In both of these functions, for some reason, i need to explicitly say the "1" is an unsigned 64 bit integer (llu) */
void pmm_bitmap_alloc(size_t block)
{
	if(pmm_bitmap_is_free(block) && block < g_pmm_total_blocks)
	{
		PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] |= 1llu << (block % PMM_BITMAP_BITS_IN_ENTRY);
		--g_pmm_free_blocks;
		++g_pmm_used_blocks;
	}
}

void pmm_bitmap_free(size_t block)
{
	if(!pmm_bitmap_is_free(block) && block < g_pmm_total_blocks)
	{
		PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] &= ~(1llu << (block % PMM_BITMAP_BITS_IN_ENTRY));
		++g_pmm_free_blocks;
		--g_pmm_used_blocks;
	}
}

bool pmm_bitmap_is_free(size_t block)
{
	return (PMM_BITMAP[block / PMM_BITMAP_BITS_IN_ENTRY] & (1llu << (block % PMM_BITMAP_BITS_IN_ENTRY))) == 0;
}

bool pmm_bitmap_is_free_blocks(size_t start_block, size_t count)
{
	/* 
	 * This function has 4 steps (very similar to pmm_bitmap_alloc_blocks)
	 * 	- 1 => Check if <start_block> and <count> both start and end at an entry index, if so, use full entry checks (uint64_t).
	 *	- 2 => Check the bits in the first entry, so <start_block> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, check the bits left in the entry)
	 *	- 3 => Check if we can use full entry checks to check full bitmap entries.
	 *	- 4 => Check the unaligned bits in the last entry, as they were not checked in the full entry loops.
	 */

	if(start_block + count > g_pmm_total_blocks)
		return false;

	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;	/* The entry index for <start_block> in the bitmap */
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;			/* The bit offset in <entry_index> for the first bit to check */
	
	size_t full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can check using full entry checks. */
	if(bit_index == 0 && full_entries > 0)
	{
		for(size_t i = 0; i < full_entries; ++i)
			if(PMM_BITMAP[entry_index + i] != 0)
				return false;
		
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	} else
	{
		size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, count);
		if((PMM_BITMAP[entry_index] & (((1llu << first_entry_blocks) - 1) << bit_index)) != 0)
			return false;

		++entry_index;
		count -= first_entry_blocks;
		start_block += first_entry_blocks;
	}

	/* 
	 * Here, we know the <start_block> will be aligned to an entry in the bitmap. 
	 * Also, if count is zero, the rest of the code wont have any effect.
	 */

	full_entries = count / PMM_BITMAP_BITS_IN_ENTRY;
	if(full_entries > 0)
	{
		for(size_t i = 0; i < full_entries; ++i)
			if(PMM_BITMAP[entry_index + i] != 0)
				return false;
		
		entry_index += full_entries;
		count -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	if(count > 0)
		if((PMM_BITMAP[entry_index] & ((1llu << count) - 1)) != 0)
			return false;
	
	return true;
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

	if(start_block + count > g_pmm_total_blocks)
		return;

	if(pmm_bitmap_is_free_blocks(start_block, count) == false)
		return;

	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;	/* The entry index for <start_block> in the bitmap */
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;			/* The bit offset in <entry_index> for the first bit to allocate */
	size_t to_alloc = count;										/* <count> Will be used at the end, so dont touch it */

	/* If <start_block> and <to_alloc> both start and end at an entry index, we can just memset the memory to 1 */
	size_t full_entries = to_alloc / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can set using memset. */
	if(bit_index == 0 && full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		to_alloc -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, to_alloc);	/* The amount of bits to set in the first entry */
		PMM_BITMAP[entry_index] |= ((1llu << first_entry_blocks) - 1) << bit_index;		/* Set <first_entry_blocks> bits, then shift them to their index in the entry */

		++entry_index;											/* Finished the first entry, so go to the next one */
		to_alloc -= first_entry_blocks;							/* Decrease the amount of blocks left to allocate */
		start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */
	}

	/* 
	 * Here, we know the <start_block> will be aligned to an entry in the bitmap. 
	 * Also, if to_alloc is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to set full entries (uint64_t), do it. */
	full_entries = to_alloc / PMM_BITMAP_BITS_IN_ENTRY;		/* The amount of full entries we can set using memset. */
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0xFF, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		to_alloc -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Allocate the blocks that were not allocated in the memset */
	PMM_BITMAP[entry_index] |= (1llu << to_alloc) - 1llu;			/* Set the remaining bits in the last entry */

	g_pmm_free_blocks -= count;
	g_pmm_used_blocks += count;
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

	if(start_block + count > g_pmm_total_blocks)
		return;

	if(pmm_bitmap_is_free_blocks(start_block, count) == true)
		return;

	size_t entry_index = start_block / PMM_BITMAP_BITS_IN_ENTRY;	/* The entry index for <start_block> in the bitmap */
	int bit_index = start_block % PMM_BITMAP_BITS_IN_ENTRY;			/* The bit offset in <entry_index> for the first bit to free */
	size_t to_free = count;											/* <count> Will be used at the end, so dont touch it */

	/* If <start_block> and <to_free> both start and end at an entry index, we can just memset the memory to 0 */
	size_t full_entries = to_free / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can clear using memset. */
	if(bit_index == 0 && full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		to_free -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_blocks = MIN(PMM_BITMAP_BITS_IN_ENTRY - bit_index, to_free);	/* The amount of bits to clear in the first entry */
		PMM_BITMAP[entry_index] &= ~(((1llu << first_entry_blocks) - 1) << bit_index);	/* Clear <first_entry_blocks> bits, then shift them to their index in the entry */

		++entry_index;											/* Finished the first entry, so go to the next one */
		to_free -= first_entry_blocks;							/* Decrease the amount of blocks left to free */
		start_block += first_entry_blocks;						/* Increase start_block so it points to the next block */
	}

	/* 
	 * Here, we know the <start_block> will be aligned to an entry in the bitmap. 
	 * Also, if to_free is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to clear full entries (uint64_t), do it. */
	full_entries = to_free / PMM_BITMAP_BITS_IN_ENTRY;			/* The amount of full entries we can clear using memset. */
	if(full_entries > 0)
	{
		memset(&PMM_BITMAP[entry_index], 0, full_entries * PMM_BITMAP_BYTES_IN_ENTRY);
		entry_index += full_entries;
		to_free -= full_entries * PMM_BITMAP_BITS_IN_ENTRY;
		start_block += full_entries * PMM_BITMAP_BITS_IN_ENTRY;
	}

	/* Free the blocks that were not freed in the memset */
	PMM_BITMAP[entry_index] &= ~((1llu << to_free) - 1llu);		/* Clear the remaining bits in the last entry */

	g_pmm_free_blocks += count;
	g_pmm_used_blocks -= count;
}

size_t pmm_bitmap_addr_to_block(phys_addr_t address)
{
	return address / PMM_BLOCK_SIZE;
}

phys_addr_t pmm_bitmap_block_to_addr(size_t block)
{
	return block * PMM_BLOCK_SIZE;
}

size_t pmm_bitmap_find_free(size_t start_block)
{
	if(start_block >= g_pmm_total_blocks)
		return -1;

	if(g_pmm_free_blocks == 0)
		return -1;

	for(size_t i = start_block / PMM_BITMAP_BITS_IN_ENTRY; i < PMM_BITMAP_LENGTH; ++i)
	{
		if (PMM_BITMAP[i] != -1llu)
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