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

int vmm_map_virtual_page(virt_addr_t address, uint64_t flags)
{
	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	virt_addr_t vaddr = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	int status = vmm_map_virtual_to_physical(vaddr, paddr, flags);
	if(status != SUCCESS)
		return status;

	return SUCCESS;
}

int vmm_map_virtual_pages(virt_addr_t address, uint64_t flags, size_t count)
{
	virt_addr_t aligned_addr = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	for(virt_addr_t vaddr = aligned_addr; vaddr < aligned_addr + count * VMM_PAGE_SIZE; vaddr += VMM_PAGE_SIZE)
	{
		int status = vmm_map_virtual_page(vaddr, flags);
		if(status != SUCCESS)
			return status;
	}
	return SUCCESS;
}

int vmm_map_virtual_to_physical(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags)
{
	uint64_t* pml4e = vmm_get_pml4e(vaddr);
	if(!vmm_is_valid_entry(*pml4e))
		vmm_alloc_pml4e(vaddr, VMM_PAGE_P | VMM_PAGE_RW);

	/* 
	 * vmm_get_pdpe cant return NULL, because the pdp was created with vmm_init_entry.
	 * Same for the next paging structures.
	 */
	uint64_t* pdpe = vmm_get_pdpe(vaddr);
	if(!vmm_is_valid_entry(*pdpe))
		vmm_alloc_pdpe(vaddr, VMM_PAGE_P | VMM_PAGE_RW);

	uint64_t* pde = vmm_get_pde(vaddr);
	if(!vmm_is_valid_entry(*pde))
		vmm_alloc_pde(vaddr, VMM_PAGE_P | VMM_PAGE_RW);
	
	phys_addr_t aligned_paddr = ALIGN_DOWN(paddr, PMM_BLOCK_SIZE);
	if(pmm_is_free(aligned_paddr))
		pmm_alloc_address(aligned_paddr, 1);

	uint64_t* pte = vmm_get_pte(vaddr);
	*pte = VMM_CREATE_TABLE_ENTRY(flags, aligned_paddr);
	*pde = VMM_INC_ENTRY_LU(*pde);

	return SUCCESS;
}

bool vmm_is_valid_entry(uint64_t entry)
{
	return entry & VMM_PAGE_P;
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

int vmm_alloc_pte(virt_addr_t address, uint64_t flags)
{
	uint64_t* pte = vmm_get_pte(address);
	if(pte == NULL)
		return ERR_PAGE_NOT_MAPPED;

	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	*pte = VMM_CREATE_TABLE_ENTRY(flags, paddr);

	uint64_t* pde = vmm_get_pde(address);
	*pde = VMM_INC_ENTRY_LU(*pde);

	return SUCCESS;
}

int vmm_free_pte(virt_addr_t address)
{
	uint64_t* pde = vmm_get_pde(address);
	uint64_t* pte = vmm_get_pte(address);

	/* If the pte is not null, the pde cannot be null. */
	if(pte == NULL)
		return ERR_PAGE_NOT_MAPPED;

	/* Free the physical block the entry points to, and mark the entry as not preset. Basicaly free it. */
	pmm_free(VMM_GET_ENTRY_TABLE(*pte));
	*pte = 0llu;

	/* 
	 * Decrease the amount of used pt entries, in the pd entry. (LU bits). 
	 * If the amount of used entries is 0, free the pd entry.
	 */
	*pde = VMM_DEC_ENTRY_LU(*pde);
	if(VMM_GET_ENTRY_LU(*pde) == 0)
		vmm_free_pde(address);

	return SUCCESS;
}

void vmm_set_pde(virt_addr_t address, uint64_t entry)
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return;
	
	*pde = entry;
}

int vmm_alloc_pde(virt_addr_t address, uint64_t flags)
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return ERR_PAGE_NOT_MAPPED;

	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	*pde = VMM_CREATE_TABLE_ENTRY(flags, paddr);

	uint64_t* pdpe = vmm_get_pdpe(address);
	*pdpe = VMM_INC_ENTRY_LU(*pdpe);

	return SUCCESS;
}

int vmm_free_pde(virt_addr_t address)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	uint64_t* pde = vmm_get_pde(address);

	/* If the pde is not null, the pdpe cannot be null. */
	if(pde == NULL)
		return ERR_PAGE_NOT_MAPPED;

	/* 
	 * Free each pt entry under the pt table the pde points to. 
	 * The vmm_free_pte function will attempt to decrement the LU count in this pde, 
	 * so if its zero it will call this function, to free the pde. As we only want to free the pt entries, without any recursion,
	 * set the value of the LU bits in this entry to 1023, so there will be no recursion. (LU will not be zero)
	 */
	int pt_to_free_count = VMM_GET_ENTRY_LU(*pde);
	*pde = VMM_SET_ENTRY_LU(*pde, (size_t)1023);
	uint64_t* pt = (uint64_t*)VMM_GET_ENTRY_TABLE(*pde);
	for(int i = 0; i < VMM_PAGE_TABLE_LENGTH; ++i)
	{
		/* 
		 * If there are no more pt's to free, break out of the loop. 
		 * This if statement not right after decrementing <pt_to_free_count> 
		 * so it handles the case when <pt_to_free_count> is 0 before even entering the loop.
		 */
		if(pt_to_free_count == 0)							
			break;		

		if(vmm_is_valid_entry(pt[i]))
		{
			vmm_free_pte(VMM_VADDR_SET_PT_IDX(address, i));
			--pt_to_free_count;
		}
	}
	
	/* Free the physical block the entry points to, and mark the entry as not preset. Basicaly free it. */
	pmm_free(VMM_GET_ENTRY_TABLE(*pde));
	*pde = 0llu;

	/* 
	 * Decrease the amount of used pd entries, in the pdp entry. (LU bits). 
	 * If the amount of used entries is 0, free the pdp entry.
	 */
	*pdpe = VMM_DEC_ENTRY_LU(*pdpe);
	if(VMM_GET_ENTRY_LU(*pdpe) == 0)
		vmm_free_pdpe(address);

	return SUCCESS;
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

int vmm_alloc_pdpe(virt_addr_t address, uint64_t flags)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe == NULL)
		return ERR_PAGE_NOT_MAPPED;

	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	*pdpe = VMM_CREATE_TABLE_ENTRY(flags, paddr);

	uint64_t* pml4e = vmm_get_pml4e(address);
	*pml4e = VMM_INC_ENTRY_LU(*pml4e);

	return SUCCESS;
}

int vmm_free_pdpe(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	uint64_t* pdpe = vmm_get_pdpe(address);

	if(pdpe == NULL)
		return ERR_PAGE_NOT_MAPPED;

	/* 
	 * Free each pd entry under the pd table the pdpe points to. 
	 * The vmm_free_pde function will attempt to decrement the LU count in this pdpe, 
	 * so if its zero it will call this function, to free the pdpe. As we only want to free the pd entries, without any recursion,
	 * set the value of the LU bits in this entry to 1023, so there will be no recursion. (LU will not be zero)
	 */
	int pd_to_free_count = VMM_GET_ENTRY_LU(*pdpe);
	*pdpe = VMM_SET_ENTRY_LU(*pdpe, (size_t)1023);
	uint64_t* pd = (uint64_t*)VMM_GET_ENTRY_TABLE(*pdpe);
	for(int i = 0; i < VMM_PAGE_TABLE_LENGTH; ++i)
	{
		/* 
		 * If there are no more pd's to free, break out of the loop. 
		 * This if statement not right after decrementing <pd_to_free_count> 
		 * so it handles the case when <pd_to_free_count> is 0 before entering the loop.
		 */
		if(pd_to_free_count == 0)							
			break;											

		if(vmm_is_valid_entry(pd[i]))
		{
			vmm_free_pde(VMM_VADDR_SET_PD_IDX(address, i));
			--pd_to_free_count;
		}
	}

	/* Free the physical block the entry points to, and mark the entry as not preset. Basicaly free it. */
	pmm_free(VMM_GET_ENTRY_TABLE(*pdpe));
	*pdpe = 0llu;

	/* 
	 * Decrease the amount of used pdp entries, in the pml4 entry. (LU bits). 
	 * If the amount of used entries is 0, free the physical block that was used for the pdp.
	 */
	*pml4e = VMM_DEC_ENTRY_LU(*pml4e);
	if(VMM_GET_ENTRY_LU(*pml4e) == 0)
		vmm_free_pml4e(address);

	return SUCCESS;
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

int vmm_alloc_pml4e(virt_addr_t address, uint64_t flags)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	
	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;
	
	*pml4e = VMM_CREATE_TABLE_ENTRY(flags, paddr);

	return SUCCESS;
}

void vmm_free_pml4e(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	pmm_free(VMM_GET_ENTRY_TABLE(*pml4e));
}

uint64_t* vmm_get_pml4()
{
	return (uint64_t*)read_cr3();
}

void vmm_set_pml4(uint64_t* pml4)
{
	write_cr3((uint64_t)pml4);
}