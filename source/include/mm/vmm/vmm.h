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
#include "error.h"
#include <stdint.h>
#include <stddef.h>

extern uint64_t* g_pml4;

typedef uint64_t virt_addr_t;

#define VMM_PAGE_SIZE 				PMM_BLOCK_SIZE
#define VMM_PAGE_TABLE_LENGTH 		512

/* 
 * Reverse memory mapping. Maps physical memory to virtual memory, using 4KiB memory blocks.
 * For example, to get the virtual address of 0x13000, take its block number (0x13000 / 4096) = 0x13 = 19 and use it in the map.
 * virt_addr_t vaddr = VMM_REVERSE_MAP[0x13000 / VMM_PAGE_SIZE];
 */
#define VMM_REVERSE_MAP				((virt_addr_t*)PMM_BITMAP_END_ADDRESS)	
#define VMM_REVERSE_MAP_LENGTH		((g_pmm_mmap_blocks / VMM_PAGE_SIZE) * sizeof(virt_addr_t))
#define VMM_REVERSE_MAP_SIZE		(VMM_REVERSE_MAP_LENGTH * sizeof(virt_addr_t))
#define VMM_REVERSE_MAP_END			(VMM_REVERSE_MAP + VMM_REVERSE_MAP_LENGTH)

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

#define VMM_VADDR_PML4E_IDX(vaddr) 			(((vaddr) >> 39) & 0x1FF)
#define VMM_VADDR_PDPE_IDX(vaddr) 			(((vaddr) >> 30) & 0x1FF)
#define VMM_VADDR_PD_IDX(vaddr)				(((vaddr) >> 21) & 0x1FF)
#define VMM_VADDR_PT_IDX(vaddr)				(((vaddr) >> 12) & 0x1FF)

#define VMM_VADDR_SET_PT_IDX(vaddr, idx)	(((vaddr) & ~((virt_addr_t)0x1FF << 12)) | ((idx) & 1023))
#define VMM_VADDR_SET_PD_IDX(vaddr, idx)	(((vaddr) & ~((virt_addr_t)0x1FF << 21)) | ((idx) & 1023))
#define VMM_VADDR_SET_PDP_IDX(vaddr, idx)	(((vaddr) & ~((virt_addr_t)0x1FF << 30)) | ((idx) & 1023))
#define VMM_VADDR_SET_PML4_IDX(vaddr, idx)	(((vaddr) & ~((virt_addr_t)0x1FF << 39)) | ((idx) & 1023))

/* Get a pointer to the paging structure/physical block the entry points to. */
#define VMM_GET_ENTRY_TABLE(entry) 					((entry) & 0x7FFFFFFFFF000)

/* Set the lower paging structure/physical block of the given entry. */
#define VMM_SET_ENTRY_TABLE(entry, table_addr) 		(((entry) & 0xFFF0000000000FFF) | ((table_addr) & 0x7FFFFFFFFF000))

/* Create a table entry, set its flags and the physical address of the table/block it points to. */
#define VMM_CREATE_TABLE_ENTRY(flags, table_paddr)	(((flags) & 0xFFF0000000000FFF) | ((table_paddr) & 0xFFFFFFFFFF000))

/* 
 * Get/set/inc/dec the value of the "Lower Used" Bits.
 * Used in the "available" bits (52-62) of an entry, to count the amount of used entries in the table the entry points to.
 * For example, in a page directory entry, these bits will count the amount of used entries in the page table the entry points to.
 */
#define VMM_GET_ENTRY_LU(entry) 		(((entry) >> 52) & 0x3FF)
#define VMM_SET_ENTRY_LU(entry, count) 	(((entry) & 0xC00FFFFFFFFFFFFF) | (((count) & 0x3FF) << 52))
#define VMM_INC_ENTRY_LU(entry)			(VMM_SET_ENTRY_LU((entry), VMM_GET_ENTRY_LU((entry)) + 1))
#define VMM_DEC_ENTRY_LU(entry)			(VMM_SET_ENTRY_LU((entry), VMM_GET_ENTRY_LU((entry)) - 1))

/* 
 * For information about virtual address translation (the structure of a 64bit pointer), (again, me in 5 minutes forgetting everything)
 * or information about the paging structures (pte, pde, pdpe, etc..)
 * visit AMD64 Architecture Programmer’s Manual Volume 2, page 142 (203 in the PDF)
 * https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf
 */

/* Initialize the virtual memory manager. Returns 0 on success, an error code otherwise. */
int vmm_init();

/* Allocates a memory block of <VMM_PAGE_SIZE>, returns its virtual address. Returns -1 on failure. */
virt_addr_t vmm_alloc_page(uint64_t flags);

/* Allocates <count> memory blocks of <VMM_PAGE_SIZE>, returns their virtual address. Returns -1 on failure. */
virt_addr_t vmm_alloc_pages(uint64_t flags, size_t count);

/* 
 * Unmaps a virtual address. 
 * Frees its corresponding physical address using the physical memory manager. Returns 0 on success, an error code otherwise.
 */
int vmm_unmap_page(virt_addr_t address);

/* 
 * Unmaps <count> pages of the given virtual address.
 * Frees their corresponding physical address using the physical memory manager. Returns 0 on success, an error code otherwise.
 */
int vmm_unmap_pages(virt_addr_t address, size_t count);

/* Find a free page, meaning, a free virtual address of VMM_PAGE_SIZE bytes. Returns -1 on failure. */
virt_addr_t vmm_find_free_page();

/* Find <count> free pages, meaning, a free virtual address of VMM_PAGE_SIZE*<count> bytes. Returns -1 on failure. */
virt_addr_t vmm_find_free_pages(size_t count);

/* Returns the physical address of the given virtual address. Returns -1 on failure. */
phys_addr_t vmm_get_physical_of(virt_addr_t address);

/* Returns the virtual address of the given physical address. Returns -1 on failure. */
virt_addr_t vmm_get_virtual_of(phys_addr_t address);

void vmm_set_virtual_of(phys_addr_t paddr, virt_addr_t vaddr);

/* 
 * Checks is the given virtual address is mapped to some physical address. 
 * Returns true if not mapped (free), false if mapped (not free). 
 */
bool vmm_is_free_page(virt_addr_t address);

/*
 * Maps the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory block, and allocates it. 
 * Returns 0 on success, an error code otherwise.
 */
int vmm_map_virtual_page(virt_addr_t address, uint64_t flags);

/*
 * Maps <count> pages of the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory blocks, and allocates them. 
 * Returns 0 on success, an error code otherwise.
 */
int vmm_map_virtual_pages(virt_addr_t address, uint64_t flags, size_t count);

/* 
 * Map a virtual address to a physical address, set the given flags for the lowest page table (only for the PTE). 
 * Uses the physical memory manager to allocate the physical address. Returns 0 on success, an error code otherwise.
 */
int vmm_map_virtual_to_physical_page(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);

/* 
 * Map a <count> pages of a virtual address to a physical address, 
 * set the given flags for the lowest page table (only for the PTE). 
 * Uses the physical memory manager to allocate the physical addresses. Returns 0 on success, an error code otherwise.
 */
int vmm_map_virtual_to_physical_pages(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags, size_t count);

/* 
 * Checks if the entry is valid.
 * meaning, if it points to a page table/physical block. Returns true if it does point to something, false otherwise.
 */
bool vmm_is_valid_entry(uint64_t entry);

/* 
 * From a given paging entry (pde, pdpe, pml4e), returns a pointer to its sub table. 
 * For a pte, the function will return a virtual address, which represents the physical block the pte points to.
 */
uint64_t* vmm_get_sub_table(uint64_t entry);

/* Returns a pointer to the page table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pte(virt_addr_t address);

/* Sets the page table entry of a given virtual address. */
void vmm_set_pte(virt_addr_t address, uint64_t entry);

/* Allocates the pt entry, updates the LU bits in the entries pd entry. Returns 0 on success, an error code otherwise. */
int vmm_alloc_pte(virt_addr_t address, uint64_t flags);

/* Frees the pt entry. Updates the LU bits in the entries pd entry. Returns 0 on success, an error code otherwise. */
int vmm_free_pte(virt_addr_t address);

/* Returns a pointer to the page directory entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pde(virt_addr_t address);

/* Sets the page directory entry of a given virtual address. */
void vmm_set_pde(virt_addr_t address, uint64_t entry);

/* Allocates the pd entry, updates the LU bits in the entries pdp entry. Returns 0 on success, an error code otherwise. */
int vmm_alloc_pde(virt_addr_t address, uint64_t flags);

/* Frees the pd entry. Updates the LU bits in the entries pdp entry. Returns 0 on success, an error code otherwise. */
int vmm_free_pde(virt_addr_t address);

/* Returns a pointer to the page directory pointer table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pdpe(virt_addr_t address);

/* Sets the page directory pointer table entry of a given virtual address. */
void vmm_set_pdpe(virt_addr_t address, uint64_t entry);

/* Allocates the pdp entry, updates the LU bits in the entries pml4 entry. Returns 0 on success, an error code otherwise. */
int vmm_alloc_pdpe(virt_addr_t address, uint64_t flags);

/* Frees the pdp entry. Updates the LU bits in the entries pml4 entry. Returns 0 on success, an error code otherwise. */
int vmm_free_pdpe(virt_addr_t address);

/* Returns a pointer to the page map level 4 entry of a given virtual address. */
uint64_t* vmm_get_pml4e(virt_addr_t address);

/* Sets the page map level 4 entry of a given virtual address. */
void vmm_set_pml4e(virt_addr_t address, uint64_t entry);

/* Allocates the pml4 entry. Returns 0 on success, an error code otherwise. */
int vmm_alloc_pml4e(virt_addr_t address, uint64_t flags);

/* Frees the pml4 entry. */
void vmm_free_pml4e(virt_addr_t address);
