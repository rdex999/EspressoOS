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

%define SAVED_GENERAL_REGS_STACK_SIZE (15 * 8)
%define SAVED_REGS_STACK_SIZE (SAVED_GENERAL_REGS_STACK_SIZE)

global isr_exception_page_fault

extern interrupt_page_fault

%macro ISR_SAVE_GENERAL_REGS 0
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro ISR_RESTORE_GENERAL_REGS 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro ISR_SAVE_REGS 0
	ISR_SAVE_GENERAL_REGS
%endmacro

%macro ISR_RESTORE_REGS 0
	ISR_RESTORE_GENERAL_REGS
%endmacro

section .text

run_exception_handler:
	ISR_SAVE_REGS

	mov rdi, [rsp + SAVED_REGS_STACK_SIZE + 8]
	call [rsp + SAVED_REGS_STACK_SIZE]

	ISR_RESTORE_REGS
	add rsp, 8 + 8
	iretq

isr_exception_page_fault:
	push interrupt_page_fault
	jmp run_exception_handler
