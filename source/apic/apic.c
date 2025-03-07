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

#include "apic/apic.h"

#include <stdlib.h>
#include <cpuid.h>
#include "error.h"
#include "cpu.h"
#include "acpi/acpi.h"
#include "mm/vmm/vmm.h"

static apic_ioapic_descriptor_t* s_ioapic_descriptor;

int apic_init()
{
	int status;

	uint32_t cpuid_unused, edx;
	__get_cpuid(1, &cpuid_unused, &cpuid_unused, &cpuid_unused, &edx);
	if((edx & CPUID_FEATURE_EDX_APIC) == 0)
		return ERR_APIC_NOT_SUPPORTED;
	
	acpi_madt_t* madt = (acpi_madt_t*)acpi_find_table_copy(ACPI_MADT_SIGNATURE);
	if(!madt)
		return ERR_ACPI_MADT_NOT_FOUND;
	
	pic8259_disable();

	for(
		acpi_madt_record_header_t* record = madt->records; 
		(uint64_t)record < (uint64_t)madt + madt->header.size; 
		*(uint8_t**)&record += record->size
	)
	{
		switch(record->type)
		{
		case ACPI_MADT_TYPE_IOAPIC:
		{
			acpi_madt_record_ioapic_t* ioapic_record = (acpi_madt_record_ioapic_t*)record;
			status = apic_ioapic_init(ioapic_record);
			if(status != SUCCESS)
				goto cleanup;

			break;
		}
		
		default:
			break;
		}
	}

	status = SUCCESS;

cleanup:
	free(madt);

	return status;
}

void pic8259_disable()
{
	/* Mask all interrupts on the master and slave PIC's */
    outb(PIC8259_MASTER_IO_DATA, 0xff);
    outb(PIC8259_SLAVE_IO_DATA, 0xff);
}

int apic_ioapic_init(acpi_madt_record_ioapic_t* ioapic_record)
{
	if(!ioapic_record)
		return ERR_INVALID_PARAMETER;
	
	apic_ioapic_descriptor_t* descriptor = (apic_ioapic_descriptor_t*)malloc(sizeof(apic_ioapic_descriptor_t));
	if(!descriptor)
		return ERR_OUT_OF_MEMORY;

	phys_addr_t ioapic_phys_base = (phys_addr_t)ioapic_record->ioapic_address;
	descriptor->first_gsi = ioapic_record->global_system_interrupt_base;

	/* 
	 * Map the configuration space of the IO APIC. If already mapped, use its virtual address. 
	 * The configuration space of an IO APIC is guaranteed to be 4KiB aligned, see intel specification.
	 */
	virt_addr_t page = vmm_get_virtual_of(ioapic_phys_base);
	if(page == (virt_addr_t)-1)
		descriptor->mmio = (uint8_t*)vmm_map_physical_page(ioapic_phys_base, VMM_PAGE_P | VMM_PAGE_RW);
	else
		descriptor->mmio = (uint8_t*)page;

	if((virt_addr_t)descriptor->mmio == (virt_addr_t)-1)	
		return ERR_OUT_OF_MEMORY;

	/* Insert the descriptor to the beginning of the IO APIC list */
	if(!s_ioapic_descriptor)
		descriptor->next = NULL;
	else
		descriptor->next = s_ioapic_descriptor;

	s_ioapic_descriptor = descriptor;

	return SUCCESS;
}

uint32_t apic_ioapic_read32(const apic_ioapic_descriptor_t* ioapic, uint8_t reg)
{
	uint32_t relative_reg = reg;
	if(reg >= APIC_IOAPIC_REG_IRQ_0)
		relative_reg -= ioapic->first_gsi;		/* If reading a redirection entry, calculate the index for this IO APIC */

	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)relative_reg;
	return *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
}

uint64_t apic_ioapic_read64(const apic_ioapic_descriptor_t* ioapic, uint8_t reg)
{
	uint32_t relative_reg = reg;
	uint32_t low, high;
	if(reg >= APIC_IOAPIC_REG_IRQ_0)
	{
		relative_reg -= ioapic->first_gsi;		/* If reading a redirection entry, calculate the index for this IO APIC */

		*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)relative_reg;
		low = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);

		*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)relative_reg + 1;
		high = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
	}
	else
	{
		*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)relative_reg;
		low = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
		high = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
	}

	return (uint64_t)low | ((uint64_t)high << 32);
}