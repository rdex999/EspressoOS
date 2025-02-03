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

#include "mm/pmm/pmm.h"
#include "cpu.h"
#include <stdint.h>
#include <stddef.h>

typedef uint64_t virt_addr_t;

#define VMM_PAGE_SIZE 					PMM_BLOCK_SIZE
#define VMM_PAGE_TABLE_LENGTH 			512

#define VMM_VADDR_PML4E_IDX(vaddr) 		(((vaddr) >> 39) & 0x1FF)
#define VMM_VADDR_PDPE_IDX(vaddr) 		(((vaddr) >> 30) & 0x1FF)
#define VMM_VADDR_PD_IDX(vaddr)			(((vaddr) >> 21) & 0x1FF)
#define VMM_VADDR_PT_IDX(vaddr)			(((vaddr) >> 12) & 0x1FF)

/* Get a pointer to the paging structure/physical block the entry points to. */
#define VMM_GET_ENTRY_TABLE(entry) 					((entry) & 0x7FFFFFFFFF000)

/* Set the lower paging structure/physical block of the given entry. */
#define VMM_SET_ENTRY_TABLE(entry, table_addr) 		(((entry) & 0xFFF0000000000FFF) | ((table_addr) & 0x7FFFFFFFFF000))

#define VMM_CREATE_TABLE_ENTRY(flags, table_paddr)	(((flags) & 0xFFF0000000000FFF) | ((table_paddr) & 0xFFFFFFFFFF000))

/* 
 * Page entry flags, for detailes (future me who forgets all of that) 
 * visit the AMD64 Architecture Programmer’s Manual Volume 2, page 154 (215 in PDF)
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 * Or the osdev wiki
 * https://wiki.osdev.org/Paging
 */
#define VMM_PAGE_P				(1 << 0)	/* Preset */
#define VMM_PAGE_RW				(1 << 1)	/* Read Write - 0 is read only, 1 is read write. */
#define VMM_PAGE_US				(1 << 2)	/* User/Supervisor - 0 for supervisor only, 1 for both user and supervisor. */
#define VMM_PAGE_PWT			(1 << 3)	/* Page WriteThrough - 0 enable writeback caching policy, 1 disable writeback caching policy. */
#define VMM_PAGE_PCD			(1 << 4)	/* Page Cache Disable */
#define VMM_PAGE_A				(1 << 5)	/* Accessed - Will be set to 1 by the CPU is the page was read/wrrited to. */
#define VMM_PAGE_D				(1 << 6)	/* Dirty - Will be set to 1 by the CPU is the page was wrriten to. */
#define VMM_PAGE_PS				(1 << 7)	/* Page Size */
#define VMM_PAGE_PTE_PAT		(1 << 7)	/* Page Attribute Table, for page table entries */
#define VMM_PAGE_G				(1 << 8)	/* Global */
#define VMM_PAGE_PDE_PDPE_PAT	(1 << 12)	/* Page Attribute Table, for page directory entries and page directory pointer table entries. */
#define VMM_PAGE_NX 			(1 << 63)	/* No Execute, for page table entries */

/* 
 * For information about virtual address translation (the structure of a 64bit pointer), (again, me in 5 minutes forgetting everything)
 * or information about the paging structures (pte, pde, pdpe, etc..)
 * visit AMD64 Architecture Programmer’s Manual Volume 2, page 142 (203 in the PDF)
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */


/* Initialize the virtual memory manager */
void vmm_init();

/* Allocates a memory block of <VMM_PAGE_SIZE>, returns its virtual address. Returns null on failure. */
virt_addr_t vmm_alloc_page();

/* Allocates <count> memory blocks of <VMM_PAGE_SIZE>, returns their virtual address. Returns null on failure. */
virt_addr_t vmm_alloc_pages(size_t count);

/* Frees a memory block of <VMM_PAGE_SIZE>. */
void vmm_free_page(virt_addr_t address);

/* Frees <count> memory blocks of <VMM_PAGE_SIZE>. */
void vmm_free_pages(virt_addr_t address, size_t count);

/* Checks is the given virtual address is mapped to some physical address. Returns true if not mapped (free), false if mapped (not free). */
bool vmm_is_free_page(virt_addr_t address);

/*
 * Maps the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory block, and allocates it. 
 * Returns 0 on success, an error code otherwise.
 */
int vmm_map_page(virt_addr_t address, uint64_t flags);

/*
 * Maps <count> pages of the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory blocks, and allocates them. 
 * Returns 0 on success, an error code otherwise.
 */
int vmm_map_pages(virt_addr_t address, uint64_t flags, size_t count);

/* 
 * Map a virtual address to a physical address, set the given flags for the lowest page table (only for the PTE). 
 * Uses the physical memory manager to allocate the physical address. Returns 0 on success, an error code otherwise.
 */
int vmm_map_virtual_to_physical(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);

/* 
 * Checks if the entry is valid.
 * meaning, if it points to a page table/physical block. Returns true if it does point to something, false otherwise.
 */
bool vmm_is_valid_entry(uint64_t entry);

/* 
 * Initializes the given entry (non PS). Allocates space for for a lower table/physical block, sets flags. 
 * Returns 0 on success, error code otherwise.
 */
int vmm_init_entry(uint64_t* entry, uint64_t flags);

/* 
 * Unmaps a virtual address. 
 * Frees its corresponding physical address using the physical memory manager.
 */
void vmm_unmap_page(virt_addr_t address);

/* 
 * Unmaps <count> pages of the given virtual address.
 * Frees their corresponding physical address using the physical memory manager. 
 */
void vmm_unmap_pages(virt_addr_t address, size_t count);

/* Returns a pointer to the page table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pte(virt_addr_t address);

/* Sets the page table entry of a given virtual address. */
void vmm_set_pte(virt_addr_t address, uint64_t entry);

/* Returns a pointer to the page directory entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pde(virt_addr_t address);

/* Sets the page directory entry of a given virtual address. */
void vmm_set_pde(virt_addr_t address, uint64_t entry);

/* Returns a pointer to the page directory pointer table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pdpe(virt_addr_t address);

/* Sets the page directory pointer table entry of a given virtual address. */
void vmm_set_pdpe(virt_addr_t address, uint64_t entry);

/* Returns a pointer to the page map level 4 entry of a given virtual address. */
uint64_t* vmm_get_pml4e(virt_addr_t address);

/* Sets the page map level 4 entry of a given virtual address. */
void vmm_set_pml4e(virt_addr_t address, uint64_t entry);

/* Returns the address of the page map level 4 paging structure. (The value of the CR3 register) */
uint64_t* vmm_get_pml4();

/* Sets a new page map level 4 table. (the value of the CR3 register) */
void vmm_set_pml4(uint64_t* pml4);