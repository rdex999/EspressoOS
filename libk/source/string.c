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

#include "string.h"

/* TODO: Optimize this function. */
void* memset(void* dest, int ch, size_t size)
{
	unsigned char* d = (unsigned char*)dest;
	for(size_t i = 0; i < size; i++)
		d[i] = (unsigned char)ch;
	return dest;
}

int memcmp(const void* lhs, const void* rhs, size_t count)
{
	const uint8_t* l = (const uint8_t*)lhs;
	const uint8_t* r = (const uint8_t*)rhs;
	for(size_t i = 0; i < count; ++i)
	{
		if(l[i] < r[i])
			return -1;
		else if(l[i] > r[i])
			return 1;
	}
	return 0;
}