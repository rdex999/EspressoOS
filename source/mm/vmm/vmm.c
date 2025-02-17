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

static virt_addr_t s_vmm_temp_map;

int vmm_init()
{
	new(&g_vmm_alloc_map) bitmap(VMM_ALLOC_MAP, VMM_ALLOC_MAP_SIZE);

	/* Allocate the physical memory of the kernel, including the reverse mapping. +1 for page map level 4. */
	phys_addr_t identity_map_end = ALIGN_UP((size_t)VMM_ALLOC_MAP_END, VMM_PAGE_SIZE);

	phys_addr_t kernel_page_tables_end = vmm_init_first_tables(identity_map_end);
	if(kernel_page_tables_end == (phys_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	int status = vmm_temp_map_init(kernel_page_tables_end);
	if(status != SUCCESS)
		return status;
	
	write_cr3((phys_addr_t)g_vmm_pml4);
	
	return SUCCESS;
}

phys_addr_t vmm_init_first_tables(phys_addr_t end_address)
{
	end_address += VMM_PAGE_SIZE;
	size_t blocks = end_address / VMM_PAGE_SIZE;
	pmm_alloc_address(0, blocks);
	vmm_mark_alloc_virtual_pages((virt_addr_t)0, blocks);
	g_vmm_pml4 = (uint64_t*)(end_address - VMM_PAGE_SIZE);
	
	for(phys_addr_t address = 0; address < end_address; address += VMM_PAGE_SIZE)
	{
		pmm_alloc_address(address, 1);
		vmm_mark_alloc_virtual_page(address);
		vmm_set_virtual_of(address, address);

		uint64_t* pml4e = vmm_get_pml4e(address);
		uint64_t* pdp;
		if(vmm_is_valid_entry(*pml4e))
			pdp = (uint64_t*)VMM_GET_ENTRY_TABLE(*pml4e);
		else
		{
			phys_addr_t pdp_paddr = pmm_alloc();
			if(pdp_paddr == (phys_addr_t)-1)
				return (phys_addr_t)-1;

			end_address += VMM_PAGE_SIZE;
			*pml4e = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pdp_paddr);
			pdp = (uint64_t*)pdp_paddr;		
		}
	
		uint64_t* pdpe = &pdp[VMM_VADDR_PDPE_IDX(address)];
		uint64_t* pd;
		if(vmm_is_valid_entry(*pdpe))
			pd = (uint64_t*)VMM_GET_ENTRY_TABLE(*pdpe);
		else
		{
			phys_addr_t pd_paddr = pmm_alloc();
			if(pd_paddr == (phys_addr_t)-1)
				return (phys_addr_t)-1;
			
			end_address += VMM_PAGE_SIZE;
			*pdpe = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pd_paddr);
			*pml4e = VMM_INC_ENTRY_LU(*pml4e);
			pd = (uint64_t*)pd_paddr;
		}
	
		uint64_t* pde = &pd[VMM_VADDR_PDE_IDX(address)];
		uint64_t* pt;
		if(vmm_is_valid_entry(*pde))
			pt = (uint64_t*)VMM_GET_ENTRY_TABLE(*pde);
		else
		{
			phys_addr_t pt_paddr = pmm_alloc();
			if(pt_paddr == (phys_addr_t)-1)
				return (phys_addr_t)-1;
			
			end_address += VMM_PAGE_SIZE;
			*pde = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pt_paddr);
			*pdpe = VMM_INC_ENTRY_LU(*pdpe);
			pt = (uint64_t*)pt_paddr;
		}

		uint64_t* pte = &pt[VMM_VADDR_PTE_IDX(address)];
		*pde = VMM_INC_ENTRY_LU(*pde);
		*pte = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, address);
	}

	return end_address;
}



virt_addr_t vmm_alloc_page(uint64_t flags)
{
	return vmm_alloc_pages(flags, 1);
}

virt_addr_t vmm_alloc_pages(uint64_t flags, size_t count)
{
	virt_addr_t address = vmm_alloc_virtual_pages(count);
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
	size_t block = vmm_address_to_block(address);
	return g_vmm_alloc_map.is_clear(block);
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
	virt_addr_t vaddr = vmm_alloc_virtual_pages(count);
	if(vaddr == (virt_addr_t)-1)
		return (virt_addr_t)-1;
	
	int status = vmm_map_virtual_to_physical_pages(vaddr, address, flags, count);
	if(status != SUCCESS)
		return (virt_addr_t)-1;
	
	return vaddr;
}

int vmm_map_virtual_to_physical_page(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags)
{
	pmm_alloc_address(paddr, 1);
	vmm_mark_alloc_virtual_page(vaddr);
	
	phys_addr_t paddr_to_map[3] = { (phys_addr_t)0 };
	
	uint64_t* pml4e = vmm_get_pml4e(vaddr);
	uint64_t* pdp;
	bool pdp_temp_map = false;
	if(vmm_is_valid_entry(*pml4e))
		pdp = vmm_get_sub_table(*pml4e);
	else
	{
		phys_addr_t pdp_paddr = pmm_alloc();
		if(pdp_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		paddr_to_map[0] = pdp_paddr;
		*pml4e = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pdp_paddr);
		pdp = (uint64_t*)vmm_temp_map(pdp_paddr);
		pdp_temp_map = true;
	}
	
	uint64_t* pdpe = &pdp[VMM_VADDR_PDPE_IDX(vaddr)];
	uint64_t* pd;
	bool pd_temp_map = false;
	if(vmm_is_valid_entry(*pdpe))
		pd = vmm_get_sub_table(*pdpe);
	else
	{
		phys_addr_t pd_paddr = pmm_alloc();
		if(pd_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		paddr_to_map[1] = pd_paddr;
		*pdpe = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pd_paddr);
		*pml4e = VMM_INC_ENTRY_LU(*pml4e);
		
		pd = (uint64_t*)vmm_temp_map(pd_paddr);
		pd_temp_map = true;
	}
	
	uint64_t* pde = &pd[VMM_VADDR_PDE_IDX(vaddr)];
	uint64_t* pt;
	bool pt_temp_map = false;
	if(vmm_is_valid_entry(*pde))
		pt = vmm_get_sub_table(*pde);
	else
	{
		phys_addr_t pt_paddr = pmm_alloc();
		if(pt_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		paddr_to_map[2] = pt_paddr;
		*pde = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pt_paddr);
		*pdpe = VMM_INC_ENTRY_LU(*pdpe);
		
		pt = (uint64_t*)vmm_temp_map(pt_paddr);
		pt_temp_map = true;
	}
	
	uint64_t* pte = &pt[VMM_VADDR_PTE_IDX(vaddr)];
	*pte = VMM_CREATE_TABLE_ENTRY(flags, paddr);
	*pde = VMM_INC_ENTRY_LU(*pde);
	vmm_set_virtual_of(paddr, vaddr);
	
	if(pt_temp_map)		vmm_temp_unmap((virt_addr_t)pd);
	if(pd_temp_map)		vmm_temp_unmap((virt_addr_t)pd);
	if(pdp_temp_map)	vmm_temp_unmap((virt_addr_t)pdp);
	
	for(int i = 0; i < (int)ARR_LEN(paddr_to_map); ++i)
	{
		if(paddr_to_map[i] != 0)
			if(vmm_map_physical_page(paddr_to_map[i], VMM_PAGE_P | VMM_PAGE_RW) == (virt_addr_t)-1)
				return ERR_OUT_OF_MEMORY;
	}
	
	return SUCCESS;
}

int vmm_map_virtual_to_physical_pages(virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags, size_t count)
{
	vmm_mark_alloc_virtual_pages(vaddr, count);
	pmm_alloc_address(paddr, count);
	
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

int vmm_temp_map_init(virt_addr_t temp_map_address)
{
	uint64_t* pml4e = vmm_get_pml4e(temp_map_address);
	uint64_t* pdp;
	if(vmm_is_valid_entry(*pml4e))
		pdp = (uint64_t*)VMM_GET_ENTRY_TABLE(*pml4e);
	else
	{
		phys_addr_t pdp_paddr = pmm_alloc();
		if(pdp_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;

		*pml4e = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pdp_paddr);
		pdp = (uint64_t*)pdp_paddr;		
	}

	uint64_t* pdpe = &pdp[VMM_VADDR_PDPE_IDX(temp_map_address)];
	uint64_t* pd;
	if(vmm_is_valid_entry(*pdpe))
		pd = (uint64_t*)VMM_GET_ENTRY_TABLE(*pdpe);
	else
	{
		phys_addr_t pd_paddr = pmm_alloc();
		if(pd_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		*pdpe = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pd_paddr);
		*pml4e = VMM_INC_ENTRY_LU(*pml4e);
		pd = (uint64_t*)pd_paddr;
	}

	uint64_t* pde = &pd[VMM_VADDR_PDE_IDX(temp_map_address)];
	if(!vmm_is_valid_entry(*pde))
	{
		phys_addr_t pt_paddr = pmm_alloc();
		if(pt_paddr == (phys_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		*pde = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, pt_paddr);
		*pdpe = VMM_INC_ENTRY_LU(*pdpe);
	}

	*pde = VMM_INC_ENTRY_LU(*pde);
	vmm_mark_alloc_virtual_pages(temp_map_address, 0 + 3);
	s_vmm_temp_map = temp_map_address;
	return SUCCESS;
}

virt_addr_t vmm_temp_map(phys_addr_t address)
{
	virt_addr_t vaddr = s_vmm_temp_map;
	s_vmm_temp_map += VMM_PAGE_SIZE;

	uint64_t pte = VMM_CREATE_TABLE_ENTRY(VMM_PAGE_P | VMM_PAGE_RW, address);
	vmm_set_pte(vaddr, pte);
	return vaddr;
}

void vmm_temp_unmap(virt_addr_t address)
{
	s_vmm_temp_map -= VMM_PAGE_SIZE;
	vmm_set_pte(address, (uint64_t)0);
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
	if(block == (size_t)-1)
		return (size_t)-1;
	
	return vmm_block_to_address(block);
}

virt_addr_t vmm_alloc_virtual_pages(size_t count)
{
	size_t block = g_vmm_alloc_map.allocate(count);
	if(block == (size_t)-1)
		return (size_t)-1;
	
	return vmm_block_to_address(block);
}

void vmm_mark_free_virtual_page(virt_addr_t address)
{
	size_t block = vmm_address_to_block(address);
	g_vmm_alloc_map.free(block);
}

void vmm_mark_free_virtual_pages(virt_addr_t address, size_t count)
{
	size_t block = vmm_address_to_block(address);
	g_vmm_alloc_map.free(block, count);
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

	size_t pt_index = VMM_VADDR_PTE_IDX(address);

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
	vmm_mark_alloc_virtual_page(address);

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
	phys_addr_t frame = VMM_GET_ENTRY_TABLE(*pte);
	pmm_free(frame);
	vmm_set_virtual_of(frame, (virt_addr_t)0);
	vmm_mark_free_virtual_page(address);
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
	
	size_t pde_index = VMM_VADDR_PDE_IDX(address);

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
