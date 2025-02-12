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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "common.h"

typedef uint64_t bitmap_entry_t;

#define BITMAP_ENTRY_BITS (sizeof(bitmap_entry_t) * 8)

class bitmap
{
public:
	/* Initializes the bitmap, clears all bits. Note: <size> is the size of the bitmap in bytes. */
	bitmap(void* buffer, size_t size);

	/* Set a single bit in the bitmap, or <count> bits starting from <index> */
	void set(size_t index);
	void set(size_t index, size_t count);

	/* clear a single bit in the bitmap, or <count> bits starting from <index> */
	void clear(size_t index);
	void clear(size_t index, size_t count);

	/* Check bit <index> is clear (0), or check if all bits starting from <index> are clear. */
	bool is_clear(size_t index) const;
	bool is_clear(size_t index, size_t count) const;

	/*  Get the index of the first clear bit, or get the index of the first <count> clear bits. */
	size_t find_clear() const;
	size_t find_clear(size_t count) const;

	inline size_t set_count() const;
	inline size_t clear_count() const;

private:
	/* Find the first clear bit starting from <index>. */	
	size_t find_clear_from(size_t index) const;

	bitmap_entry_t* const m_buffer;
	const size_t m_size;
	const size_t m_bit_count;

	size_t m_clear;
	size_t m_set;
};