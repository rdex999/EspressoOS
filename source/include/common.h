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

// Works only when align is a power of 2.
// Basicaly, when subtracting 1 from align, all bits from right, until the original bit of align, are set to 1. So, we get a mask So, we get a mask.
// So add the mask to the address, that way the address will "have" the mask.
// Then, remove remove the mask from the address so it will be aligned upwards.
#define ALIGN(address, align) (address + (align - 1)) & ~(align - 1)