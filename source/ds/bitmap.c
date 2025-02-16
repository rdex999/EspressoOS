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
	if(!is_clear(index))
		return;

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

	if(!is_clear(index, count))								/* If there is at least on set bit, return. */
		return;

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
	if(is_clear(index))
		return;
	
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

	if(is_clear(index, count))
		return;

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

bool bitmap::is_clear(size_t index, size_t count) const
{
	/* 
	 * This function has 4 steps (very similar to bitmap::set(size_t index, size_t count))
	 * 	- 1 => Check if <index> and <count> both start and end at an entry index, if so, use full entry checks (uint64_t).
	 *	- 2 => Check the bits in the first entry, so <index> will be aligned to a bitmap entry. 
	 * 		   (If count is greater than the amount of bits left in the entry, check the bits left in the entry)
	 *	- 3 => Check if we can use full entry checks to check full bitmap entries.
	 *	- 4 => Check the unaligned bits in the last entry, as they were not checked in the full entry loops.
	 */

	if(index + count > m_bit_count)
		return false;

	size_t entry_index = index / BITMAP_ENTRY_BITS;		/* The entry index for <index> in the bitmap */
	int bit_offset = index % BITMAP_ENTRY_BITS;			/* The bit offset in <entry_index> for the first bit to check */
	
	size_t full_entries = count / BITMAP_ENTRY_BITS;	/* The amount of full entries we can check using full entry checks. */
	if(bit_offset == 0 && full_entries > 0)
	{
		for(size_t i = 0; i < full_entries; ++i)
			if(m_buffer[entry_index + i] != 0)
				return false;
		
		entry_index += full_entries;
		count -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	} else
	{
		size_t first_entry_blocks = MIN(BITMAP_ENTRY_BITS - bit_offset, count);
		if((m_buffer[entry_index] & (((1llu << first_entry_blocks) - 1) << bit_offset)) != 0)
			return false;

		++entry_index;
		count -= first_entry_blocks;
		index += first_entry_blocks;
	}

	/* 
	 * Here, we know the <index> will be aligned to an entry in the bitmap. 
	 * Also, if count is zero, the rest of the code wont have any effect.
	 */

	full_entries = count / BITMAP_ENTRY_BITS;
	if(full_entries > 0)
	{
		for(size_t i = 0; i < full_entries; ++i)
			if(m_buffer[entry_index + i] != 0)
				return false;
		
		entry_index += full_entries;
		count -= full_entries * BITMAP_ENTRY_BITS;
		index += full_entries * BITMAP_ENTRY_BITS;
	}

	if(count > 0)
		if((m_buffer[entry_index] & ((1llu << count) - 1)) != 0)
			return false;
	
	return true;
}

inline size_t bitmap::find_clear() const
{
	return find_clear_from(0);
}

size_t bitmap::find_clear(size_t count) const
{
	size_t index = find_clear_from(0);
	while(index != (size_t)-1)
	{
		if(is_clear(index, count))
			return index;

		index = find_clear_from(index + count);
	}
	return -1;
}

size_t bitmap::find_clear_from(size_t index) const
{
	if(index >= m_bit_count)
		return -1;

	if(m_clear == 0)
		return -1;

	size_t entry_index = index / BITMAP_ENTRY_BITS;
	size_t bit_offset = index % BITMAP_ENTRY_BITS;

	/* 
	 * If <index> is not aligned to a bitmap entry, manualy check the first entry for clear bits. 
	 * If it has a clear bit, return its bit index. If not, proceed and loop over the entries.
	 */
	if(bit_offset != 0)
	{
		bitmap_entry_t first = m_buffer[entry_index] | ((1llu << bit_offset) - 1llu);
		if(first != (bitmap_entry_t)-1)
		{
			size_t offset = __builtin_ffsll(~first) - 1;	/* ffsll counts from 1, so subtract 1 */
			return entry_index * BITMAP_ENTRY_BITS + offset;
		}
		++entry_index;
	}

	for(size_t i = entry_index; i < m_size / sizeof(bitmap_entry_t); ++i)
	{
		if (m_buffer[i] != -1llu)
		{
			/* 
			 * The __builtin_ffsll function performs the BSF (byte scan forward) instruction, 
			 * which finds the index of the first set bit. However, __builtin_ffsll increases the result of BSF by 1
			 * so we need to subtract 1 to get the bit index.
			 * Here we find the first bit that is set in the result of the bitwise NOT operation on the bitmap entry.
			 * (Which is, the first cleared bit in the bitmap entry)
			 */
			size_t offset = __builtin_ffsll(~m_buffer[i]) - 1;	/* ffsll counts from 1, so subtract 1 */
			return i * BITMAP_ENTRY_BITS + offset;
		}
	}
	return -1;
}

size_t bitmap::allocate()
{
	size_t index = find_clear();
	if(index == (size_t)-1)
		return (size_t)-1;
	
	set(index);
	return index;
}

size_t bitmap::allocate(size_t count)
{
	size_t index = find_clear(count);
	if(index == (size_t)-1)
		return (size_t)-1;

	set(index, count);
	return index;
}

inline void bitmap::free(size_t index)
{
	clear(index);
}

inline void bitmap::free(size_t index, size_t count)
{
	clear(index, count);
}

inline size_t bitmap::set_count() const
{
	return m_set;
}

inline size_t bitmap::clear_count() const
{
	return m_clear;
}