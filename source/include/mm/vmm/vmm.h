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

#define VMM_PML4E_GET_PDPT(pml4e) 		((pml4e) & 0x7FFFFFFFFF000)

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

/*
 * Maps the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory block, and allocates it. 
 */
void vmm_map_page(virt_addr_t address, uint64_t flags);

/*
 * Maps <count> pages of the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory blocks, and allocates them. 
 */
void vmm_map_pages(virt_addr_t address, uint64_t flags, size_t count);

/* 
 * Map a virtual address to a physical address, set the given flags. 
 * Uses the physical memory manager to allocate the physical address
 */
void vmm_map_virtual_to_physical(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);

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

/* Returns a pointer to the page map level 4 entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pml4e(virt_addr_t address);

/* Sets the page map level 4 entry of a given virtual address. */
void vmm_set_pml4e(virt_addr_t address, uint64_t entry);

/* Returns the address of the page map level 4 paging structure. (The value of the CR3 register) */
uint64_t* vmm_get_pml4();

/* Sets a new page map level 4 table. (the value of the CR3 register) */
void vmm_set_pml4(uint64_t* pml4);