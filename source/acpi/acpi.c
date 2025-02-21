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

static void* s_acpi_rsdt = NULL;

int acpi_init(multiboot_info_t* mbd)
{
	return acpi_init_rsdp(mbd);
}

int acpi_init_rsdp(multiboot_info_t* mbd)
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

		/* TODO: Check the signature of the xsdp */

		s_acpi_rsdt = (void*)xsdp->xsdt_address;
		return SUCCESS;
	}
	/* If the old_tag is not NULL and the RSDP table it points to is ACPI V1 */
	else if(old_tag && ((acpi_rsdp_t*)old_tag->rsdp)->revision == 0)
	{
		acpi_rsdp_t* rsdp = (acpi_rsdp_t*)&old_tag->rsdp;

		if(!acpi_is_table_valid(rsdp, sizeof(acpi_rsdp_t)))
			return ERR_ACPI_TABLE_CHECKSUM;
		
		s_acpi_rsdt = (void*)((uint64_t)rsdp->rsdt_address && 0xFFFFFFFF);
		return SUCCESS;
	}

	return ERR_ACPI_TABLE_CHECKSUM;
}

bool acpi_is_table_valid(void* table, size_t size)
{
	uint8_t sum = 0;
	uint8_t* byte_table = (uint8_t*)table;
	for (size_t i = 0; i < size; ++i)
		sum += byte_table[i];
	
	return sum == 0;
}