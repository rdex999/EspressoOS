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

#include "acpi/acpi.h"

static void* s_acpi_root_sdt = NULL;
static int s_acpi_version = -1;

int acpi_init(multiboot_info_t* mbd)
{
	return acpi_init_rsdt(mbd);
}

int acpi_init_rsdt(multiboot_info_t* mbd)
{
	multiboot_tag_old_acpi_t* old_tag = (multiboot_tag_old_acpi_t*)mbd->find_tag(MULTIBOOT_TAG_TYPE_ACPI_OLD);
	multiboot_tag_new_acpi_t* new_tag = (multiboot_tag_new_acpi_t*)mbd->find_tag(MULTIBOOT_TAG_TYPE_ACPI_NEW);

	if(!old_tag && !new_tag)
		return ERR_ACPI_RSDP_NOT_FOUND;

	/* If the new_tag is not NULL and the RSDP table it points to is ACPI V2 */
	if(new_tag && ((acpi_xsdp_t*)new_tag->rsdp)->rsdp.revision == 2)
	{
		acpi_xsdp_t* xsdp = (acpi_xsdp_t*)&new_tag->rsdp;

		if(!acpi_is_table_valid(xsdp, sizeof(acpi_rsdp_t)))
			return ERR_ACPI_TABLE_CHECKSUM;

		if(!acpi_is_table_valid((uint8_t*)xsdp + sizeof(acpi_rsdp_t), sizeof(acpi_xsdp_t) - sizeof(acpi_rsdp_t)))
			return ERR_ACPI_TABLE_CHECKSUM;

		if(memcmp(xsdp->rsdp.signature, ACPI_RSDP_SIGNATURE, 8) != 0)
			return ERR_ACPI_RSDP_NOT_FOUND;

		int status = acpi_map_sdt(xsdp->xsdt_address, &s_acpi_root_sdt, NULL);
		if(status != SUCCESS)
			return status;

		if(memcmp(((acpi_xsdt_t*)s_acpi_root_sdt)->header.signature, ACPI_XSDT_SIGNATURE, 4) != 0)
			return ERR_ACPI_TABLE_NOT_FOUND;
		
		s_acpi_version = 2;
	}
	/* If the old_tag is not NULL and the RSDP table it points to is ACPI V1 */
	else if(old_tag && ((acpi_rsdp_t*)old_tag->rsdp)->revision == 0)
	{
		acpi_rsdp_t* rsdp = (acpi_rsdp_t*)&old_tag->rsdp;

		if(!acpi_is_table_valid(rsdp, sizeof(acpi_rsdp_t)))
			return ERR_ACPI_TABLE_CHECKSUM;
	
		if(memcmp(rsdp->signature, ACPI_RSDP_SIGNATURE, 8) != 0)
			return ERR_ACPI_RSDP_NOT_FOUND;			

		int status = acpi_map_sdt((phys_addr_t)rsdp->rsdt_address & 0xFFFFFFFF, &s_acpi_root_sdt, NULL);
		if(status != SUCCESS)
			return status;

		if(memcmp(((acpi_rsdt_t*)s_acpi_root_sdt)->header.signature, ACPI_RSDT_SIGNATURE, 4) != 0)
			return ERR_ACPI_TABLE_NOT_FOUND;

		s_acpi_version = 1;
	}
	else
		return ERR_ACPI_RSDP_NOT_FOUND;

	if(!acpi_is_table_valid(s_acpi_root_sdt, ((acpi_rsdt_t*)s_acpi_root_sdt)->header.size))
		return ERR_ACPI_TABLE_CHECKSUM;

	return SUCCESS;
}

int acpi_map_sdt(phys_addr_t sdt, void** mapped_sdt, size_t* page_count)
{
	/* 
	 * The actual type of the SDT is unknown for know, but we know each SDT has a header, 
	 * which containes the size of the whole SDT.
	 * So, map the physical address of the SDT, and use just enough pages so we can access the SDT header.
	 * Then, using the mapped address access the header and get the size of the whole SDT.
	 * If we need to map more pages to cover the full size, do it. If not, dont.
	 */

	size_t pages = VMM_ADDRESS_SIZE_PAGES(sdt, sizeof(acpi_sdt_header_t));
	virt_addr_t mapped_pages = vmm_map_physical_pages(sdt, VMM_PAGE_P | VMM_PAGE_RW, pages);
	if(mapped_pages == (virt_addr_t)-1)
		return ERR_OUT_OF_MEMORY;

	acpi_sdt_header_t* sdt_header = (acpi_sdt_header_t*)(mapped_pages + sdt % VMM_PAGE_SIZE);
	if(pages * VMM_PAGE_SIZE - sdt % VMM_PAGE_SIZE < sdt_header->size)
	{
		size_t sdt_size = sdt_header->size;

		int status = vmm_unmap_pages(mapped_pages, pages);
		if(status != SUCCESS)
			return status;

		pages = VMM_ADDRESS_SIZE_PAGES(sdt, sdt_size);
		mapped_pages = vmm_map_physical_pages(sdt, VMM_PAGE_P | VMM_PAGE_RW, pages);
		if(mapped_pages == (virt_addr_t)-1)
			return ERR_OUT_OF_MEMORY;
		
		sdt_header = (acpi_sdt_header_t*)(mapped_pages + sdt % VMM_PAGE_SIZE);
	}

	if(mapped_sdt)
		*mapped_sdt = sdt_header;
	
	if(page_count)
		*page_count = pages;

	return SUCCESS;
}

int acpi_unmap_sdt(void* mapped_sdt)
{
	if(!mapped_sdt)
		return ERR_INVALID_PARAMETER;

	size_t pages = VMM_ADDRESS_SIZE_PAGES((virt_addr_t)mapped_sdt, ((acpi_sdt_header_t*)mapped_sdt)->size);
	return vmm_unmap_pages((virt_addr_t)mapped_sdt, pages);
}

int acpi_find_table(const char* signature, void** table, size_t* page_count)
{
	if(!signature || ! table || ! page_count)
		return ERR_INVALID_PARAMETER;
	
	int status;
	acpi_sdt_header_t* sdt;
	size_t pages;
	if(s_acpi_version == 2)
	{
		acpi_xsdt_t* xsdt = (acpi_xsdt_t*)s_acpi_root_sdt;
		int length = (xsdt->header.size - sizeof(xsdt->header)) / sizeof(xsdt->sdt_pointers[0]);
		for(int i = 0; i < length; ++i)
		{
			status = acpi_map_sdt(xsdt->sdt_pointers[i], (void**)&sdt, &pages);
			if(status != SUCCESS)
				return status;

			if(memcmp(((acpi_sdt_header_t*)sdt)->signature, signature, 4) == 0 && acpi_is_table_valid(sdt, sdt->size))
			{
				*table = sdt;
				*page_count = pages;
				return SUCCESS;
			}
			status = vmm_unmap_pages((virt_addr_t)sdt, pages);
			if(status != SUCCESS)
				return status;
		}
	}
	else if(s_acpi_version == 1)
	{
		acpi_rsdt_t* rsdt = (acpi_rsdt_t*)s_acpi_root_sdt;
		int length = (rsdt->header.size - sizeof(rsdt->header)) / sizeof(rsdt->sdt_pointers[0]);
		for(int i = 0; i < length; ++i)
		{
			status = acpi_map_sdt((phys_addr_t)rsdt->sdt_pointers[i] & 0xFFFFFFFF, (void**)&sdt, &pages);
			if(status != SUCCESS)
				return status;

			if(memcmp(((acpi_sdt_header_t*)sdt)->signature, signature, 4) == 0 && acpi_is_table_valid(sdt, sdt->size))
			{
				*table = sdt;
				*page_count = pages;
				return SUCCESS;
			}
			status = vmm_unmap_pages((virt_addr_t)sdt, pages);
			if(status != SUCCESS)
				return status;
		}
	}
	else
		return ERR_ACPI_NOT_INITIALIZED;
	
	return ERR_ACPI_TABLE_NOT_FOUND;
}

bool acpi_is_table_valid(void* table, size_t size)
{
	uint8_t sum = 0;
	uint8_t* byte_table = (uint8_t*)table;
	for (size_t i = 0; i < size; ++i)
		sum += byte_table[i];
	
	return sum == 0;
}