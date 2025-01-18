;
;	---------------- [ BOOT CODE ] ----------------
;

; This file containes the multiboot2 header, and the first and most basic initialization code for the operating system.
bits 32
global start

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

; When getting here, the following registers hold some special values.
; 	EAX => Should hold the magic number 36D76289h, which indicates the OS was loaded by a multiboot2 compliant bootloader.
;	EBX	=> Should point to the multiboot2 information structure (physical address)
start:
	cmp eax, 36D76289h						; Check magic number
	jne exit 								; If the magic number is invalid, error out and exit.

	mov esp, stack_top						; Set up the stack.
	push ebx								; Save multiboot2 information structure address.

	call check_long_mode

exit:
	jmp 0xFFFF:0							; Far jump to the reset vector, this will reboot the PC
	jmp exit								; This command should not run, but if it does then just continue looping doing nothing


; Checks for long mode, jumps to the "exit" lable if long mode isnt supported.
check_long_mode:
	; This function has three parts.
	; Part 1	=> Check for CPUID
	; Part 2	=> Check for CPUID extended functions
	; Part 3 	=> Check for long mode support

	; 	PART 1
	; Check for CPUID by trying to flip the 21st bit of EFLAGS. If it stays the same if CPUID is available.
	pushfd									; Push the value of EFLAGS so we can get it
	pop eax									; Get the value of EFLAGS
	xor eax, 1 << 21						; Flip 21st bit
	mov ebx, eax							; Save modified value, for checking if it has stayed the same

	push eax								; Push the modified value of EFLAGS, so we can write it to EFLAGS
	popfd									; Write new value into EFLAGS

	pushfd									; Push the value of EFLAGS so we can get it
	pop eax									; Get the value of EFLAGS after the write operation

	cmp eax, ebx							; Compare the new value of EFLAGS to the old one
	jne exit								; If its not the save, exit.

	mov dword [0A0000h], 0AABBCCDDh
	jmp $
	
	
	;	PART 2
	; Check for CPUID extended functions.
	; TODO:

