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

uint64_t* g_vmm_pml4;
bitmap g_vmm_alloc_map;

int vmm_init()
{
	new(&g_vmm_alloc_map) bitmap(VMM_ALLOC_MAP, VMM_ALLOC_MAP_SIZE);

	/* Allocate the physical memory of the kernel, including the reverse mapping. */
	size_t kernel_blocks = DIV_ROUND_UP((size_t)VMM_ALLOC_MAP_END, PMM_BLOCK_SIZE);
	pmm_alloc_address(0, kernel_blocks);
	vmm_mark_alloc_virtual_pages((virt_addr_t)0, kernel_blocks);

	phys_addr_t pml4_paddr = pmm_alloc();
	if(pml4_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	/* 
	 * This part, which initiates the first paging structures is temporary, as i still need to create some kind 
	 * of data structure for managing free virtual addresses.
	 * The reason im initializing these paging structures here, is because of a bug which my horrible design caused.
	 * When using vmm_map_physical_page/s(), it finds a free virtual address, then uses the vmm_map_virtual_to_physical_page/s()
	 * function for mapping the physical address. The problem is when the virtual address thats given to 
	 * vmm_map_virtual_to_physical_page/s() doesnt have paging structures initialized, the function will try to create them,
	 * and in the process of creating them it finds a free virtual address. The virtual address it will find, was probably also found
	 * in the first call to vmm_map_physical_page/s() because it wasnt able yet to "mark" it as allocated.
	 */
	g_vmm_pml4 = (uint64_t*)pml4_paddr;
	vmm_set_virtual_of(pml4_paddr, (virt_addr_t)g_vmm_pml4);

	phys_addr_t pdp_paddr = pmm_alloc();
	if(pdp_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	g_vmm_pml4[0] = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pdp_paddr);
	vmm_set_virtual_of(pdp_paddr, (virt_addr_t)pdp_paddr);

	phys_addr_t pd_paddr = pmm_alloc();
	if(pd_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	((uint64_t*)pdp_paddr)[0] = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pd_paddr);
	vmm_set_virtual_of(pd_paddr, (virt_addr_t)pd_paddr);

	phys_addr_t pt_paddr = pmm_alloc();
	if(pt_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	((uint64_t*)pd_paddr)[0] = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pt_paddr);
	vmm_set_virtual_of(pt_paddr, (virt_addr_t)pt_paddr);

	/* Identity map kernel memory, including the reverse memory map */
	int status = vmm_map_virtual_to_physical_pages(
		0, 
		0, 
		VMM_PAGE_P | VMM_PAGE_RW,
		(size_t)VMM_ALLOC_MAP_END / VMM_PAGE_SIZE
	);

	if(status != SUCCESS)
		return status;
	
	write_cr3((uint64_t)pml4_paddr);

	return SUCCESS;
}

virt_addr_t vmm_alloc_page(uint64_t flags)
{
	return vmm_alloc_pages(flags, 1);
}

virt_addr_t vmm_alloc_pages(uint64_t flags, size_t count)
{
	virt_addr_t address = vmm_find_free_pages(count);
	if(address == (virt_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	int status = vmm_map_virtual_pages(address, flags, count);
	if(status != SUCCESS)
		return (virt_addr_t)-1;
	
	return address;
}

int vmm_unmap_page(virt_addr_t address)
{
	virt_addr_t vaddr = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	return vmm_free_pte(vaddr);
}

int vmm_unmap_pages(virt_addr_t address, size_t count)
{
	virt_addr_t aligned = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	for(virt_addr_t vaddr = aligned; vaddr < aligned + count * VMM_PAGE_SIZE; vaddr += VMM_PAGE_SIZE)
	{
		int status = vmm_unmap_page(vaddr);
		if(status != SUCCESS)
			return status;
	}
	return SUCCESS;
}

virt_addr_t vmm_find_free_page()
{
	return vmm_find_free_pages(1llu);
}

virt_addr_t vmm_find_free_pages(size_t count)
{
	/* 
	 * Ok now hear me out - I know its a shitty algorithem (traveling the whole paging structure), 
	 * i will use some kind of data structure for managing free pages in the future.
	 */

	if(count == 0)
		return (virt_addr_t)-1;

	for(
		virt_addr_t vaddr = ALIGN_UP((virt_addr_t)VMM_REVERSE_MAP_END, VMM_PAGE_SIZE) + PMM_BLOCK_SIZE*1; 
		vaddr < (virt_addr_t)-1 - VMM_PAGE_SIZE; 
		vaddr += VMM_PAGE_SIZE
	)
	{
		uint64_t* pde = vmm_get_pde(vaddr);
		if(pde != NULL && VMM_GET_ENTRY_LU(*pde) >= VMM_PAGE_TABLE_LENGTH)
		{
			vaddr += (VMM_PAGE_TABLE_LENGTH - 1) * VMM_PAGE_SIZE;
			continue;
		}

		if(!vmm_is_free_page(vaddr))
			continue;
	
		size_t free_blocks = 0;	
		for(virt_addr_t va = vaddr; free_blocks < count; va += VMM_PAGE_SIZE, ++free_blocks)
		{
			if(!vmm_is_free_page(va))
				break;
		}
		if(free_blocks != count)
		{
			vaddr += (free_blocks - 1) * VMM_PAGE_SIZE;
			continue;
		}
		
		return vaddr;
	}

	return (virt_addr_t)-1;
}

phys_addr_t vmm_get_physical_of(virt_addr_t address)
{
	virt_addr_t aligned = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	uint64_t* pte = vmm_get_pte(aligned);
	if(pte == NULL)
		return (phys_addr_t)-1;
	
	return VMM_GET_ENTRY_TABLE(*pte) + (address - aligned);
}

virt_addr_t vmm_get_virtual_of(phys_addr_t address)
{
	size_t block = address / VMM_PAGE_SIZE;
	if(block >= VMM_REVERSE_MAP_LENGTH)
		return (virt_addr_t)-1;

	return VMM_REVERSE_MAP[block] + address % VMM_PAGE_SIZE;
}

void vmm_set_virtual_of(phys_addr_t paddr, virt_addr_t vaddr)
{
	virt_addr_t alg_vaddr = ALIGN_DOWN(vaddr, VMM_PAGE_SIZE);
	size_t index = paddr / VMM_PAGE_SIZE;
	if(index >= VMM_REVERSE_MAP_LENGTH)
		return;
	
	VMM_REVERSE_MAP[index] = alg_vaddr;
}

bool vmm_is_free_page(virt_addr_t address)
{
	virt_addr_t vaddr = ALIGN_DOWN(address, VMM_PAGE_SIZE);

	uint64_t* pte = vmm_get_pte(vaddr);
	if(pte == NULL)
		return true;

	/* If this is a valid entry, it means its allocated. So invert the result of the function. (if not valid - return true) */
	return !vmm_is_valid_entry(*pte);
}

int vmm_map_virtual_page(virt_addr_t address, uint64_t flags)
{
	phys_addr_t paddr = pmm_alloc();
	if(paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	virt_addr_t vaddr = ALIGN_DOWN(address, VMM_PAGE_SIZE);
	int status = vmm_map_virtual_to_physical_page(vaddr, paddr, flags);
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

virt_addr_t vmm_map_physical_page(phys_addr_t address, uint64_t flags)
{
	return vmm_map_physical_pages(address, flags, (size_t)1);
}

virt_addr_t vmm_map_physical_pages(phys_addr_t address, uint64_t flags, size_t count)
{
	virt_addr_t vaddr = vmm_find_free_pages(count);
	if(vaddr == (virt_addr_t)-1)
		return (virt_addr_t)-1;

	int status = vmm_map_virtual_to_physical_pages(vaddr, address, flags, count);
	if(status != SUCCESS)
		return (virt_addr_t)-1;
	
	return vaddr;
}

int vmm_map_virtual_to_physical_page(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags)
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

	uint64_t pte = VMM_CREATE_TABLE_ENTRY(flags, aligned_paddr);
	vmm_set_pte(vaddr, pte);

	*pde = VMM_INC_ENTRY_LU(*pde);

	return SUCCESS;
}

int vmm_map_virtual_to_physical_pages(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags, size_t count)
{
	for(size_t block = 0; block < count; ++block)
	{
		virt_addr_t cvaddr = vaddr + block * VMM_PAGE_SIZE;
		phys_addr_t cpaddr = paddr + block * VMM_PAGE_SIZE;
		int status = vmm_map_virtual_to_physical_page(cvaddr, cpaddr, flags);
		if(status != SUCCESS)
			return status;
	}
	return SUCCESS;
}

bool vmm_is_valid_entry(uint64_t entry)
{
	return entry & VMM_PAGE_P;
}

uint64_t* vmm_get_sub_table(uint64_t entry)
{
	phys_addr_t address = VMM_GET_ENTRY_TABLE(entry);
	return (uint64_t*)vmm_get_virtual_of(address);
}

void vmm_mark_alloc_virtual_page(virt_addr_t address)
{
	size_t block = vmm_address_to_block(address);
	g_vmm_alloc_map.set(block);
}

void vmm_mark_alloc_virtual_pages(virt_addr_t address, size_t count)
{
	size_t block = vmm_address_to_block(address);
	g_vmm_alloc_map.set(block, count);
}

virt_addr_t vmm_alloc_virtual_page()
{
	size_t block = g_vmm_alloc_map.allocate();
	return vmm_block_to_address(block);
}

virt_addr_t vmm_alloc_virtual_pages(size_t count)
{
	size_t block = g_vmm_alloc_map.allocate(count);
	return vmm_block_to_address(block);
}

virt_addr_t vmm_block_to_address(size_t block)
{
	return block * VMM_PAGE_SIZE;
}

size_t vmm_address_to_block(virt_addr_t address)
{
	return address / VMM_PAGE_SIZE;
}

uint64_t *vmm_get_pte(virt_addr_t address) 
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return NULL;

	uint64_t* pt = vmm_get_sub_table(*pde);
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
	vmm_set_virtual_of(VMM_GET_ENTRY_TABLE(entry), address);
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
	vmm_set_virtual_of(paddr, address);

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

uint64_t* vmm_get_pde(virt_addr_t address)
{
	uint64_t* pdpe = vmm_get_pdpe(address);
	if(pdpe == NULL)
		return NULL;
	
	uint64_t* pd = vmm_get_sub_table(*pdpe);
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

int vmm_alloc_pde(virt_addr_t address, uint64_t flags)
{
	uint64_t* pde = vmm_get_pde(address);
	if(pde == NULL)
		return ERR_PAGE_NOT_MAPPED;

	phys_addr_t pt_paddr = pmm_alloc();
	if(pt_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	virt_addr_t pt_vaddr = vmm_map_physical_page(pt_paddr, VMM_PAGE_P | VMM_PAGE_RW);
	if(pt_vaddr == (virt_addr_t)-1)
	{
		pmm_free(pt_paddr);
		return ERR_OUT_OF_MEMORY;
	}

	*pde = VMM_CREATE_TABLE_ENTRY(flags, pt_paddr);

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
	uint64_t* pt = vmm_get_sub_table(*pde);
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
	
	phys_addr_t pt_paddr = VMM_GET_ENTRY_TABLE(*pde);
	virt_addr_t pt_vaddr = vmm_get_virtual_of(pt_paddr);

	/* Free the physical block the entry points to, and mark the entry as not preset. Basicaly free it. */
	pmm_free(pt_paddr);

	/* Free the virtual address that was mapped to the physical address of the page table. */
	vmm_unmap_page(pt_vaddr);

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

	uint64_t* pdp = vmm_get_sub_table(*pml4e);
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

	phys_addr_t pd_paddr = pmm_alloc();
	if(pd_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	virt_addr_t pd_vaddr = vmm_map_physical_page(pd_paddr, VMM_PAGE_P | VMM_PAGE_RW);
	if(pd_vaddr == (virt_addr_t)-1)
	{
		pmm_free(pd_paddr);	
		return ERR_OUT_OF_MEMORY;
	}

	*pdpe = VMM_CREATE_TABLE_ENTRY(flags, pd_paddr);

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
	uint64_t* pd = vmm_get_sub_table(*pdpe);
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

	phys_addr_t pd_paddr = VMM_GET_ENTRY_TABLE(*pdpe);
	virt_addr_t pd_vaddr = vmm_get_virtual_of(pd_paddr);

	/* Free the physical block the entry points to, and mark the entry as not preset. Basicaly free it. */
	pmm_free(pd_paddr);

	/* Free the virtual address that the physical address of the page directory was mapped to */
	vmm_unmap_page(pd_vaddr);

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
	return &g_vmm_pml4[pml4e_index];
}

void vmm_set_pml4e(virt_addr_t address, uint64_t entry)
{
	int pml4e_index = VMM_VADDR_PML4E_IDX(address);
	g_vmm_pml4[pml4e_index] = entry;
}

int vmm_alloc_pml4e(virt_addr_t address, uint64_t flags)
{
	uint64_t* pml4e = vmm_get_pml4e(address);
	
	phys_addr_t pdp_paddr = pmm_alloc();
	if(pdp_paddr == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	virt_addr_t pdp_vaddr = vmm_map_physical_page(pdp_paddr, VMM_PAGE_P | VMM_PAGE_RW);
	if(pdp_vaddr == (virt_addr_t)-1)
	{
		pmm_free(pdp_paddr);
		return ERR_OUT_OF_MEMORY;
	}

	*pml4e = VMM_CREATE_TABLE_ENTRY(flags, pdp_paddr);

	return SUCCESS;
}

void vmm_free_pml4e(virt_addr_t address)
{
	uint64_t* pml4e = vmm_get_pml4e(address);

	phys_addr_t pdp_paddr = VMM_GET_ENTRY_TABLE(*pml4e);
	virt_addr_t pdp_vaddr = vmm_get_virtual_of(pdp_paddr);

	/* Free the physical address of the pdp. */
	pmm_free(pdp_paddr);

	/* Free the virtual address that the physical address of the pdp was mapped to. */
	vmm_unmap_page(pdp_vaddr);
}
