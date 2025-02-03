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

int vmm_map_page(virt_addr_t address, uint64_t flags)
{
	return vmm_map_pages(address, flags, 1);
}

int vmm_map_pages(virt_addr_t address, uint64_t flags, size_t count)
{
	virt_addr_t aligned_addr = ALIGN(address, VMM_PAGE_SIZE);
	for(virt_addr_t vaddr = aligned_addr; vaddr < aligned_addr + count * VMM_PAGE_SIZE; vaddr += VMM_PAGE_SIZE)
	{
		phys_addr_t paddr = pmm_alloc();
		if(paddr == (phys_addr_t)-1)
			return 1;

		int status = vmm_map_virtual_to_physical(vaddr, paddr, flags);
		if(status != 0)
			return status;
	}
	return 0;
}

int vmm_map_virtual_to_physical(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags)
{
	uint64_t* pml4e = vmm_get_pml4e(vaddr);
	if(!vmm_is_valid_entry(*pml4e))
		if(vmm_init_entry(pml4e, VMM_PAGE_P | VMM_PAGE_RW) != 0)
			return 1;		/* TODO: Handle error */

	/* 
	 * vmm_get_pdpe cant return NULL, because the pdp was created with vmm_init_entry.
	 * Same for the next paging structures.
	 */
	uint64_t* pdpe = vmm_get_pdpe(vaddr);
	if(!vmm_is_valid_entry(*pdpe))
		if(vmm_init_entry(pdpe, VMM_PAGE_P | VMM_PAGE_RW) != 0)
			return 1; 	/* TODO: Handle error */

	uint64_t* pde = vmm_get_pde(vaddr);
	if(!vmm_is_valid_entry(*pde))
		if(vmm_init_entry(pde, VMM_PAGE_P | VMM_PAGE_RW) != 0)
			return 1; 	/* TODO: Handle error */
	
	phys_addr_t aligned_paddr = ALIGN(paddr, PMM_BLOCK_SIZE);
	if(pmm_is_free(aligned_paddr))
		pmm_alloc_address(aligned_paddr, 1);

	uint64_t* pte = vmm_get_pte(vaddr);
	*pte = VMM_CREATE_TABLE_ENTRY(flags, aligned_paddr);

	return 0;
}

bool vmm_is_valid_entry(uint64_t entry)
{
	return entry & VMM_PAGE_P;
}

int vmm_init_entry(uint64_t* entry, uint64_t flags)
{
	phys_addr_t table = pmm_alloc();
	if(table == (phys_addr_t)-1)
		return 1;	/* TODO: Setup errors */

	*entry = VMM_CREATE_TABLE_ENTRY(flags, table);

	return 0;
}

uint64_t *vmm_get_pte(virt_addr_t address) 
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return NULL;

	uint64_t* pt = (uint64_t*)VMM_GET_ENTRY_TABLE(*pde);
	if(pt == NULL)
		return NULL;

	size_t pt_index = VMM_VADDR_PT_IDX(address);

	return &pt[pt_index];
}

void vmm_set_pte(virt_addr_t address, uint64_t entry)
{
	uint64_t* pte = vmm_get_pte(address);
	if(pte == NULL)
		return;
	
	*pte = entry;
}

uint64_t* vmm_get_pde(virt_addr_t address)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe == NULL)
		return NULL;
	
	uint64_t* pd = (uint64_t*)VMM_GET_ENTRY_TABLE(*pdpe);
	if(pd == NULL)
		return NULL;
	
	size_t pde_index = VMM_VADDR_PD_IDX(address);

	return &pd[pde_index];
}

void vmm_set_pde(virt_addr_t address, uint64_t entry)
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return;
	
	*pde = entry;
}

void vmm_alloc_pde(virt_addr_t address)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe != NULL)
		*pdpe = VMM_INC_ENTRY_LU(*pdpe);
}

void vmm_free_pde(virt_addr_t address)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	uint64_t* pde = vmm_get_pde(address);

	/* If the pde is not null, the pdpe cannot be null. */
	if(pde == NULL)
		return;

	/* Mark the entry as not preset, basicaly free it. */
	*pde = 0llu;

	/* 
	 * Decrease the amount of used pd entries, in the pdp entry. (LU bits). 
	 * If the amount of used entries is 0, free the pdp entry.
	 */
	*pdpe = VMM_DEC_ENTRY_LU(*pdpe);
	if(VMM_GET_ENTRY_LU(*pdpe) == 0)
		vmm_free_pdpe(address);
}

uint64_t* vmm_get_pdpe(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	if(pml4e == NULL)
		return NULL;

	uint64_t* pdp = (uint64_t*)VMM_GET_ENTRY_TABLE(*pml4e);
	if(pdp == NULL)
		return NULL;

	size_t pdp_index = VMM_VADDR_PDPE_IDX(address);

	return &pdp[pdp_index];
}

void vmm_set_pdpe(virt_addr_t address, uint64_t entry)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe == NULL)
		return;

	*pdpe = entry;
}

void vmm_alloc_pdpe(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	*pml4e = VMM_INC_ENTRY_LU(*pml4e);
}

void vmm_free_pdpe(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	uint64_t* pdpe = vmm_get_pdpe(address);

	if(pdpe == NULL)
		return;

	/* Mark the entry as not preset, basicaly free it. */
	*pdpe = 0llu;

	/* 
	 * Decrease the amount of used pdp entries, in the pml4 entry. (LU bits). 
	 * If the amount of used entries is 0, free the physical block that was used for the pdp.
	 */
	*pml4e = VMM_DEC_ENTRY_LU(*pml4e);
	if(VMM_GET_ENTRY_LU(*pml4e) == 0)
		pmm_free(VMM_GET_ENTRY_TABLE(*pml4e));
}

uint64_t* vmm_get_pml4e(virt_addr_t address)
{
	int pml4e_index = VMM_VADDR_PML4E_IDX(address);
	return &vmm_get_pml4()[pml4e_index];
}

void vmm_set_pml4e(virt_addr_t address, uint64_t entry)
{
	int pml4e_index = VMM_VADDR_PML4E_IDX(address);
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