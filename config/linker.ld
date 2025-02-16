# 
# This file is part of the EspressoOS project (https://github.com/rdex999/EspressoOS.git).
# Copyright (c) 2025 David Weizman.
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
# 
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

# This file specifies the structure of the kernel.bin file.
# Code sections, symbols, all of that goes here.

ENTRY(start)

SECTIONS
{
	# Load kernel at 1MiB in memory.
	. = 1M;

	# First, put everything from the .multiboot2 setion. It containes the multiboot2 header and tags.
	.boot : { KEEP(*(.multiboot2)) } :text

	# set _text to the .text sections start address.
	_text = .;

	# All code goes next, after the multiboot2 header
	.text : { *(.text) } :text

	# Set _etext to the .text section end address
	_etext = .;

	# After the code, put all data here (.data section)
	.data : { *(.data) } :data

	# Also needed, "rodata" stands for Read Only Data
	.rodata : { *(.rodata) } :rodata

	# Set _edata to the data sections end address
	_edata = .;

	# Next put the .bss section, its basically the same as the data section but its used  more for global variables and stuff.
	.bss : { *(.bss) } :bss

	# Set _ebss to the .bss section end address.
	_ebss = .;

	# Set _kernel_end to the kernels end address.
	_kernel_end = .;
}

# The "PHDRS" command defines program headers with specific permissions.
# The ":phdr" (:text for example) syntax associates sections with these headers (which have the permissions).
# This is done so the linker wont print a warning saying the kernel has a LOAD segment with RWX permissions.
PHDRS
{
	text 	PT_LOAD FLAGS(READ | EXECUTE);
	data 	PT_LOAD FLAGS(READ | WRITE);
	rodata 	PT_LOAD FLAGS(READ);
	bss 	PT_LOAD FLAGS(READ | WRITE);
}