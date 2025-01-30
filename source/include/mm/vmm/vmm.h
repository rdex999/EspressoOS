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
#include <stdint.h>
#include <stddef.h>

#define VMM_PAGE_SIZE PMM_BLOCK_SIZE

/* Initialize the virtual memory manager */
void vmm_init();

/* Allocates a memory block of <VMM_PAGE_SIZE>. Returns null on failure. */
uint64_t vmm_alloc_page();

/* Allocates <count> memory blocks of <VMM_PAGE_SIZE>. Returns null on failure. */
uint64_t vmm_alloc_pages(size_t count);

/* Frees a memory block of <VMM_PAGE_SIZE>. */
void vmm_free_page(uint64_t address);

/* Frees <count> memory blocks of <VMM_PAGE_SIZE>. */
void vmm_free_pages(uint64_t address);

/*
 * Maps the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory block, and allocates it. 
 */
void vmm_map_page(uint64_t virtual_address, uint64_t flags);

/*
 * Maps <count> pages of the given virtual address to a physical address, sets the given flags.
 * Uses the physical memory manager to find a free physical memory blocks, and allocates them. 
 */
void vmm_map_pages(uint64_t virtual_address, uint64_t flags, size_t count);

/* 
 * Map a virtual address to a physical address, set the given flags. 
 * Uses the physical memory manager to allocate the physical address
 */
void vmm_map_virtual_to_physical(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);

/* 
 * Unmaps a virtual address. 
 * Frees its corresponding physical address using the physical memory manager.
 */
void vmm_unmap_page(uint64_t virtual_address);

/* 
 * Unmaps <count> pages of the given virtual address.
 * Frees their corresponding physical address using the physical memory manager. 
 */
void vmm_unmap_pages(uint64_t virtual_address, size_t count);

/* Returns a pointer to the page table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pte(uint64_t virtual_address);

/* Sets the page table entry of a given virtual address. */
void vmm_set_pte(uint64_t virtual_address, uint64_t entry);

/* Returns a pointer to the page directory entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pde(uint64_t virtual_address);

/* Sets the page directory entry of a given virtual address. */
void vmm_set_pde(uint64_t virtual_address, uint64_t entry);

/* Returns a pointer to the page directory pointer table entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pdpe(uint64_t virtual_address);

/* Sets the page directory pointer table entry of a given virtual address. */
void vmm_set_pdpe(uint64_t virtual_address, uint64_t entry);

/* Returns a pointer to the page map level 4 entry of a given virtual address. Will return null on failure. */
uint64_t* vmm_get_pml4e(uint64_t virtual_address);

/* Sets the page map level 4 entry of a given virtual address. */
void vmm_set_pml4e(uint64_t virtual_address, uint64_t entry);

/* Returns the address of the page map level 4 paging structure. (The value of the CR3 register) */
uint64_t* vmm_get_pml4();

/* Sets the address of the page map level 4 paging structure. (The value of the CR3 register) */
void vmm_set_pml4(uint64_t* pml4);