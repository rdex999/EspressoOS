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

#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include "cpu.h"

unsigned int popcount64(uint64_t number)
{
	uint32_t ecx, unused;
	cpuid(CPUID_CODE_GET_FEATURES, &unused, &unused, &ecx, &unused);

	if(ecx & CPUID_FEATURE_ECX_POPCNT)					/* If the POPCNT instruction is available, use it. */
	{
		uint64_t count;
		asm volatile("popcntq %1, %0"
			: "=r"(count)
			: "r"(number)
		);
		return count;
	}
	else
	{
		/* For details about this algorithm, see https://en.wikipedia.org/wiki/Hamming_weight */	
		
		const uint64_t m1  = 0x5555555555555555; 		/* binary: 0101... */
		const uint64_t m2  = 0x3333333333333333; 		/* binary: 00110011.. */
		const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; 		/* binary:  4 zeros,  4 ones ... */
		const uint64_t h01 = 0x0101010101010101; 		/* the sum of 256 to the power of 0,1,2,3... */

		number -= (number >> 1) & m1;            		/* put count of each 2 bits into those 2 bits */
		number = (number & m2) + ((number >> 2) & m2);	/* put count of each 4 bits into those 4 bits  */
		number = (number + (number >> 4)) & m4;			/* put count of each 8 bits into those 8 bits  */
		return (number * h01) >> 56;  					/* returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
	}

}