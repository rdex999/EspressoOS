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

#include <stddef.h>
#include <stdint.h>

void* malloc(size_t size);
void free(void* ptr);

unsigned int popcount64(uint64_t number);

/* Basically just malloc, use these keywords when creating/deleting objects. */
inline void* operator new (size_t size) 			{ return malloc(size); }
inline void* operator new[] (size_t size) 			{ return malloc(size); }

inline void operator delete (void* ptr) 			{ free(ptr); }
inline void operator delete[] (void* ptr) 			{ free(ptr); }

/* Placement new operators */
inline void* operator new (size_t, void* ptr)   	{ return ptr; }
inline void* operator new[] (size_t, void* ptr) 	{ return ptr; }

/* Placement delete operators */
inline void  operator delete (void*, void*) 		{ }
inline void  operator delete[] (void*, void*) 		{ }

/* Sized delete operators */
inline void operator delete (void* ptr, size_t) 	{ free(ptr); }
inline void operator delete[] (void* ptr, size_t)	{ free(ptr); }