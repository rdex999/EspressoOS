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
; ---------------- [ LONG MODE SETUP FUNCTIONS ] ----------------
;

%define PG_PRESENT (1 << 0)
%define PG_READ_WRITE (1 << 1)

section .text

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

	;	PART 2
	; Check for CPUID extended functions.
	; After CPUID EAX=80000000h, EAX should be set to a value greater that 80000000h if extended functions are available.
	mov eax, 80000000h						; CPUID function that checks for extended functions
	cpuid									; Check for extended functions
	cmp eax, 80000001h						; Check if the returned value is greater than 80000000h, if it is then there are extended functions
	jb exit									; If there are no extended functions, exit

	;	PART 3
	; Check for long mode.
	; By using the extended function CPUID EAX=80000001h, we can check for long mode.
	; After the extended function, the LM bit (29th bit) of EDX should be 1 if long mode is available.
	mov eax, 80000001h						; Set function number
	cpuid									; Perform CPUID function
	test edx, 1 << 29						; Check if the LM bit is set
	jz exit									; If the LM bit is not set, there is no long mode so just exit.

	ret


; Initializes the page tables, identity map the first 2MiB.
setup_page_tables:
	; The page tables will be located at the following addresses:
	; PML4T => 1000h.
	; PDPT 	=> 2000h.
	; PDT 	=> 3000h.
	; PT 	=> 4000h.

	; First, clear the page tables
	mov edi, 1000h							; First PML4T address
	mov cr3, edi							; CR3 points to the first PML4T
	xor eax, eax							; Value to set pages to - null
	mov ecx, (4000h + 1000h - 1000h) / 4	; Amount of bytes to fill, divided by 4 because writing dwords
	rep stosd								; Set pages to null

	; Setup the pointers
	mov edi, cr3							; Get a pointer to the PML4T
	mov dword [edi + 0*1000h], 2000h | PG_PRESENT | PG_READ_WRITE	; PML4T[0] = PDPT
	mov dword [edi + 1*1000h], 3000h | PG_PRESENT | PG_READ_WRITE	; PDPT[0] = PDT
	mov dword [edi + 2*1000h], 4000h | PG_PRESENT | PG_READ_WRITE	; PDT[0] = PT

	; Identity map the first 2MiB by filling the first PT
	add edi, 3 * 1000h						; EDI points at the first entry of the PT 	EDI=&PT[0]
	mov ecx, 512							; Amount of entries to fill
	mov al, PG_PRESENT | PG_READ_WRITE		; Enable Preset + Read/Write
.pt_fill_loop:
	mov [edi], eax							; Initialize current PT entry to the next physical address
	add edi, 8								; Advance to the next PT entry
	add eax, 4096							; Increase current physical address
	loop .pt_fill_loop						; Continue filling the tables

	ret

; Enters 64 bit long mode.
; NOTE: After calling this function, all code after it must be 64 bit code. (do: "bits 64")
; RETURNS:
; 	- RDI	=> A pointer to the multiboot2 information structure.
enter_long_mode:
	; Enable PAE
	mov eax, cr4                 			; Get the value of CR4
	or eax, 1 << 5               			; Set the PAE-bit (5th bit in CR4)
	mov cr4, eax                 			; Write changes back to CR4

	; Enable long mode bit in the EFER MSR (MSR - Model Specific Register)
	mov ecx, 0C0000080h						; MSR to read, set it to the EFER MSR number
	rdmsr									; Read EFER, get result in EDX:EAX
	or eax, 1 << 8							; Set the LM (Long Mode) bit
	wrmsr									; Write back the changes into EFER

	; Set PG bit in CR0
	mov eax, cr0							; Get the value of CR0
	or eax, 1 << 31							; Set PG bit
	mov cr0, eax							; Write the new value back

	pop eax									; Get this functions return address in EAX, because loading the GDT messes up the stack
	pop edi									; Get the multiboot2 information structure pointer in EDI, as an argument for kernel_main

	lgdt [gdt.pointer]						; Load the 64 bit GDT
	jmp GDT_CODE_SEL:.x64					; Switch to 64 bit long mode
.x64:
	bits 64

	; Ignore NASM warnings on these two lines, its not overflowing
	and rax, 0FFFFFFFFh						; Zero out the high 32 bits as the return address is 32 bit.
	and rdi, 0FFFFFFFFh						; Zero out high 32 bits as the multiboot2 information structure pointer is 32 bits.

	mov rsp, stack_top						; Setup the stack
	jmp rax									; return from this function