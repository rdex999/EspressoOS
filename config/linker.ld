# This file specifies the structure of the kernel.bin file.
# Code sections, symbols, all of that goes here.

ENTRY(start)

SECTIONS
{
	# Load kernel at 1MiB in memory.
	. = 1M;

	# First, put everything from the .multiboot2 setion. It containes the multiboot2 header and tags.
	.boot : { KEEP(*(.multiboot2)) }

	# set _text to the .text sections start address.
	_text = .;
	# All code goes next, after the multiboot2 header
	.text : { *(.text) }
	# Set _etext to the .text section end address
	_etext = .;

	# After the code, put all data here (.data section)
	.data : { *(.data) }
	# Also needed, "rodata" stands for Read Only Data
	.rodata : { *(.rodata) }
	# Set _edata to the data sections end address
	_edata = .;

	# Next put the .bss section, its basically the same as the data section but its used  more for global variables and stuff.
	.bss : { *(.bss) }
	# Set _ebss to the .bss section end address.
	_ebss = .;

	# Set _kernel_end to the kernels end address.
	_kernel_end = .;
}