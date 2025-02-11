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

#include "ds/bitmap.h"

bitmap::bitmap(void* buffer, size_t size)
	: m_buffer((bitmap_entry_t*)buffer), m_size(size), m_bit_count(size*8)
{
	m_set = 0;
	m_clear = size * 8;
	memset(buffer, 0, size);
}

void bitmap::set(size_t index)
{
	size_t entry_idx = index / BITMAP_ENTRY_BITS;
	size_t entry_offset = index % BITMAP_ENTRY_BITS;
	m_buffer[entry_idx] |= (bitmap_entry_t)1 << entry_offset;

	++m_set;
	--m_clear;
}

void bitmap::set(size_t index, size_t count)
{
	/* 
	 * This function has 4 steps
	 * 	- 1 => Check if <index> and <count> both start and end at an entry index, if so, memset the memory to 1 and return.
	 *	- 2 => Mark the bits in the first entry, so <index> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, mark the bits left in the entry)
	 *	- 3 => Check if we can use memset to set full bitmap entries.
	 *	- 4 => Mark the unaligned bits in the last entry, as they were not cleared in the memset.
	 */

	if(index + count > m_bit_count)
		return;

	/* TODO: Implement is_set(..) */
	// if(is_set(index, count))
	// 	return;

	size_t entry_index = index / BITMAP_ENTRY_BITS;			/* The entry index for <index> in the bitmap */
	int bit_offset = index % BITMAP_ENTRY_BITS;				/* The bit offset in <entry_index> for the first bit to set */
	size_t to_set = count;									/* <count> Will be used at the end, so dont touch it */

	/* If <index> and <to_set> both start and end at an entry index, we can just memset the memory to 1 */
	size_t full_entries = to_set / BITMAP_ENTRY_BITS;		/* The amount of full entries we can set using memset. */
	if(bit_offset == 0 && full_entries > (size_t)0)
	{
		memset(&m_buffer[entry_index], 0xFF, full_entries * sizeof(bitmap_entry_t));
		entry_index += full_entries;
		to_set -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_bits = MIN(BITMAP_ENTRY_BITS - bit_offset, to_set);	/* The amount of bits to set in the first entry */
		m_buffer[entry_index] |= (((bitmap_entry_t)1 << first_entry_bits) - 1llu) << bit_offset;	/* Set <first_entry_bits> bits, then shift them to their index in the entry */

		++entry_index;											/* Finished the first entry, so go to the next one */
		to_set -= first_entry_bits;							/* Decrease the amount of bits left to allocate */
		index += first_entry_bits;							/* Increase <index> so it points to the next bit */
	}

	/* 
	 * Here, we know the <index> will be aligned to an entry in the bitmap. 
	 * Also, if to_alloc is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to set full entries (bitmap_entry_t), do it. */
	full_entries = to_set / BITMAP_ENTRY_BITS;					/* The amount of full entries we can set using memset. */
	if(full_entries > 0)
	{
		memset(&m_buffer[entry_index], 0xFF, full_entries * sizeof(bitmap_entry_t));
		entry_index += full_entries;
		to_set -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	}

	/* Set the bits that were not set in the memset */
	m_buffer[entry_index] |= ((bitmap_entry_t)1 << to_set) - (bitmap_entry_t)1;		/* Set the remaining bits in the last entry */

	m_clear -= count;
	m_set += count;
}

void bitmap::clear(size_t index)
{

	size_t entry_idx = index / BITMAP_ENTRY_BITS;
	size_t entry_offset = index % BITMAP_ENTRY_BITS;
	m_buffer[entry_idx] |= (bitmap_entry_t)1 << entry_offset;

	--m_set;
	++m_clear;
}

void bitmap::clear(size_t index, size_t count)
{
	/* 
	 * This function has 4 steps
	 * 	- 1 => Check if <index> and <count> both start and end at an entry index, if so, memset the memory to 0 and return.
	 *	- 2 => Clear the bits in the first entry, so <index> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, clear the bits left in the entry)
	 *	- 3 => Check if we can use memset to clear full bitmap entries.
	 *	- 4 => clear the unaligned bits in the last entry, as they were not cleared in the memset.
	 */

	if(index + count > m_bit_count)
		return;

	/* TODO: Implement is_clear(..) */
	// if(is_clear(index, count))
	// 	return;

	size_t entry_index = index / BITMAP_ENTRY_BITS;			/* The entry index for <index> in the bitmap */
	int bit_offset = index % BITMAP_ENTRY_BITS;				/* The bit offset in <entry_index> for the first bit to clear */
	size_t to_clear = count;									/* <count> Will be used at the end, so dont touch it */

	/* If <index> and <to_clear> both start and end at an entry index, we can just memset the memory to 1 */
	size_t full_entries = to_clear / BITMAP_ENTRY_BITS;		/* The amount of full entries we can clear using memset. */
	if(bit_offset == 0 && full_entries > (size_t)0)
	{
		memset(&m_buffer[entry_index], 0, full_entries * sizeof(bitmap_entry_t));
		entry_index += full_entries;
		to_clear -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	} else
	{
		/* Finish the first entry, then if there are more check if using memset is possible */
		size_t first_entry_bits = MIN(BITMAP_ENTRY_BITS - bit_offset, to_clear);	/* The amount of bits to clear in the first entry */
		m_buffer[entry_index] &= ~((((bitmap_entry_t)1 << first_entry_bits) - 1llu) << bit_offset);	/* Set <first_entry_bits> bits, then shift them to their index in the entry, then mask them off */

		++entry_index;											/* Finished the first entry, so go to the next one */
		to_clear -= first_entry_bits;							/* Decrease the amount of bits left to allocate */
		index += first_entry_bits;							/* Increase <index> so it points to the next bit */
	}

	/* 
	 * Here, we know the <index> will be aligned to an entry in the bitmap. 
	 * Also, if to_clear is zero, the rest of the code wont have any effect.
	 */

	/* If we can use memset to clear full entries (bitmap_entry_t), do it. */
	full_entries = to_clear / BITMAP_ENTRY_BITS;					/* The amount of full entries we can clear using memset. */
	if(full_entries > 0)
	{
		memset(&m_buffer[entry_index], 0, full_entries * sizeof(bitmap_entry_t));
		entry_index += full_entries;
		to_clear -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	}

	/* clear the bits that were not set in the memset */
	m_buffer[entry_index] &= ~(((bitmap_entry_t)1 << to_clear) - (bitmap_entry_t)1);	/* Clear the remaining bits in the last entry */

	m_clear += count;
	m_set -= count;
}

bool bitmap::is_clear(size_t index) const
{
	size_t entry_idx = index / BITMAP_ENTRY_BITS;
	size_t entry_offset = index % BITMAP_ENTRY_BITS;
	return (m_buffer[entry_idx] & ((bitmap_entry_t)1 << entry_offset)) == (bitmap_entry_t)0;
}