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

extern char _kernel_end;

/* 
 * Align up, Works only when align is a power of 2.
 * Basicaly, when subtracting 1 from align, all bits from right, until the original bit of align, are set to 1. So, we get a mask So, we get a mask.
 * So add the mask to the address, that way the address will "have" the mask.
 * Then, remove remove the mask from the address so it will be aligned upwards.
 */
#define ALIGN_UP(address, align) 		(((address) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(num, align)			((num) - (num) % (align))
#define IS_ALIGNED(num, align)			((num) % (align) == 0)
#define DIV_ROUND_UP(num, denominator) 	(((num) + (denominator) - 1) / (denominator))
#define KERNEL_END 						((void*)&_kernel_end)
#define MIN(a, b) 						((a) < (b) ? (a) : (b))
#define MAX(a, b) 						((a) > (b) ? (a) : (b))
#define ARR_LEN(arr)					(sizeof((arr)) / sizeof((arr)[0]))

/* Idk, for now i wont create a gdt.h header, as its realy rarely used. */
#define GDT_CODE_SELECTOR 8
#define GDT_DATA_SELECTOR 8