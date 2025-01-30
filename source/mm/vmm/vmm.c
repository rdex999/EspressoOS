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

#include "mm/vmm/vmm.h"

uint64_t* vmm_get_pdpe(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	if(pml4e == NULL)
		return NULL;

	uint64_t* pdpt = (uint64_t*)VMM_PML4E_GET_PDPT(*pml4e);
	if(pdpt == NULL)
		return NULL;

	size_t pdpe_index = VMM_VADDR_PDPE_IDX(address);
	if(pdpe_index >= VMM_PAGE_TABLE_LENGTH)
		return NULL;

	return &pdpt[pdpe_index];
}

void vmm_set_pdpe(virt_addr_t address, uint64_t entry)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe == NULL)
		return;

	*pdpe = entry;
}

uint64_t* vmm_get_pml4e(virt_addr_t address)
{
	int pml4e_index = VMM_VADDR_PML4E_IDX(address);
	if(pml4e_index >= VMM_PAGE_TABLE_LENGTH)
		return NULL;

	return &vmm_get_pml4()[pml4e_index];
}

void vmm_set_pml4e(virt_addr_t address, uint64_t entry)
{
	int pml4e_index = VMM_VADDR_PML4E_IDX(address);
	if(pml4e_index >= VMM_PAGE_TABLE_LENGTH)
		return;

	vmm_get_pml4()[pml4e_index] = entry;
}

uint64_t* vmm_get_pml4()
{
	return (uint64_t*)read_cr3();
}

void vmm_set_pml4(uint64_t* pml4)
{
	write_cr3((uint64_t)pml4);
}