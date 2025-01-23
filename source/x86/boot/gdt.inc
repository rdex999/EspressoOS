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
; ---------------- [ GLOBAL DESCRIPTOR TABLE MANIPULATION ] ----------------
;

%define GDT_AB_ACCESSED 	(1 << 0)	; Will be set by the CPU if the segment was accessed, unles it was set to 1 beforehand. (Just set it to 1)
%define GDT_AB_READ_WRITE	(1 << 1)	; For code segments: readable bit. For data segments: writable bit
%define GDT_AB_DC			(1 << 2)	; Direction/Conforming
%define GDT_AB_EXECUTABLE	(1 << 3)	; if 0 the entry defines a data segment, if 1 the entry defines a code segment
%define GDT_AB_CODE_DATA 	(1 << 4)	; System flag. 0 for a system segment, 1 for a code/data segment
%define GDT_AB_DPL			(1 << 5)	; (two bits) Descriptor Privilege Level - 0 for highest privilege (kernel) 3 for lowest.
%define GDT_AB_PRESENT 		(1 << 7)	; Set to 1 for a valid entry

%define GDT_FLAGS_L			(1 << 1)	; Long mode. If set, DB must be 0
%define GDT_FLAGS_DB		(1 << 2)	; Size flag. 0: 16bit protected mode. 1: 32bit protected mode.
%define GDT_FLAGS_G			(1 << 3)	; Granularity, the value the size limit is scaled by. 0 for bytes, 1 for 4KiB blocks.

%define GDT_CODE_SEL (gdt.code - gdt)
%define GDT_DATA_SEL (gdt.data - gdt)

; PARAMETERS
;   - 1) Limit, 20 bits
;   - 2) Base, 32 bits
;   - 3) Access byte
;   - 4) Flags, 4 bits
%macro CREATE_GDT_GATE 4

  dw (%1) & 0FFFFh								; Limit - low 16 bits
  dw (%2) & 0FFFFh								; Base  - low 16 bits
  db ((%2) >> 16) & 0FFh						; Base  - bits 16-23
  db (%3) & 0FFh								; Access byte
  db (((%1) >> 16) & 0Fh) | (((%4) & 0Fh) << 4)	; Limit high 4 bits in the low 4 bits, flags in the high 4 bits
  db ((%2) >> 24) & 0FFh						; Base, high 8 bits

%endmacro

section .data

gdt:
.null:
	CREATE_GDT_GATE 0, 0, 0, 0

.code:
	CREATE_GDT_GATE 																\
		0FFFFFh, 																	\
		0, 																			\
		GDT_AB_PRESENT | GDT_AB_CODE_DATA | GDT_AB_EXECUTABLE | GDT_AB_READ_WRITE, 	\
		GDT_FLAGS_L | GDT_FLAGS_G

.data:
	CREATE_GDT_GATE																	\
		0FFFFFh,																	\
		0,																			\
		GDT_AB_PRESENT | GDT_AB_CODE_DATA | GDT_AB_READ_WRITE,						\
		GDT_FLAGS_L | GDT_FLAGS_G

.pointer:
	dw $ - gdt - 1
	dd gdt