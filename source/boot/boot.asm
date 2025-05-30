; 
; This file is part of the EspressoOS project (https://github.com/rdex999/EspressoOS.git).
; Copyright (c) 2025 David Weizman.
; 
; This program is free software: you can redistribute it and/or modify  
; it under the terms of the GNU General Public License as published by  
; the Free Software Foundation, version 3.
; 
; This program is distributed in the hope that it will be useful, but 
; WITHOUT ANY WARRANTY; without even the implied warranty of 
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
; General Public License for more details.
; 
; You should have received a copy of the GNU General Public License 
; along with this program. If not, see <https://www.gnu.org/licenses/>.
;

;
;	---------------- [ BOOT CODE ] ----------------
;

; This file containes the multiboot2 header, and the first and most basic initialization code for the operating system.
bits 32
global start
extern kernel_main

%include "boot/gdt.inc"
%include "boot/lm_setup.inc"

; General multiboot2 tag.
; PARAMETERS
;	- 1 => The type of the tag.
;	- 2 => Flags for the tag.
;	- 3 => The size of the tag, in bytes.
%macro MULTIBOOT2_TAG 3
	align 8									; All multiboot2 tags must be 8 byte aligned.
	dw %1									; Type
	dw %2									; Flags
	dd %3									; Size
%endmacro

section .multiboot2
align 8										; The multiboot2 header must be 8 byte aligned, and so are all of its tags (if used)
multiboot2_header_start:
	dd 0E85250D6h							; Multiboot2 magic number
	dd 0									; Architecture - 32bit protected mode
	dd .end - multiboot2_header_start		; The size of this header

	; Checksum. A value, thats when added to the magic number, architecture, and header size, will be zero.
	; Solve for x: 	x + magic + architecture + header_size = 0
	dd 100000000h - (0E85250D6h + 0 + (.end - multiboot2_header_start))

	MULTIBOOT2_TAG 0, 0, 0					; End tag, specifies that there are to more tags.
.end:

section .bss
resb 4096									; Allocate 4096 bytes for the stack
stack_top:

section .text
bits 32

; When getting here, the following registers hold some special values.
; 	EAX => Should hold the magic number 36D76289h, which indicates the OS was loaded by a multiboot2 compliant bootloader.
;	EBX	=> Should point to the multiboot2 information structure (physical address)
start:
	cmp eax, 36D76289h						; Check magic number
	jne exit 								; If the magic number is invalid, error out and exit.

	cli										; Disable interrupts because interrupt handlers are not set up yet.
	mov esp, stack_top						; Set up the stack.
	push ebx								; Save multiboot2 information structure address.

	; TODO: For some reason, when compiling without the -g (debug information), 
	; grub places the multiboot info structure at 0x4000, like what the hell grub, realy.
	; So, in the future, copy the multiboot information structure to the end of the kernel, 
	; and set up the physical memory manager to use its bitmap at the end of the multiboot information structure.

	call check_long_mode
	call setup_page_tables
	call enter_long_mode

	bits 64

	call kernel_main

exit:
	bits 32

	jmp 0xFFFF:0							; Far jump to the reset vector, this will reboot the PC
	jmp exit								; This command should not run, but if it does then just continue looping doing nothing