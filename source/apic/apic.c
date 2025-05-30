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
#include "error.h"
#include "cpu.h"
#include "acpi/acpi.h"
#include "mm/vmm/vmm.h"

static ioapic_descriptor_t* s_ioapic_descriptor;

/* 
 * The virtual address override, represents the physical address of the mmio for all local APIC's on the system.
 * -1 means the MADT has no entry of type 5 (LAPIC address override), and we should read IA32_APIC_BASE MSR instead.
 */
static uint64_t s_lapic_address_override = (uint64_t)-1; 

int apic_init()
{
	int status;

	uint32_t cpuid_unused, edx;
	cpuid(CPUID_CODE_GET_FEATURES, &cpuid_unused, &cpuid_unused, &cpuid_unused, &edx);
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
			status = ioapic_init(ioapic_record);
			if(status != SUCCESS)
				goto cleanup;

			break;
		}

		case ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE:
		{
			/* The physical address of the local APIC configuration space if guaranteed to be 4 KiB aligned, see intel spec. */
			acpi_madt_record_lapic_address_override_t* lapic_override = (acpi_madt_record_lapic_address_override_t*)record;
			s_lapic_address_override = vmm_map_physical_page(
				lapic_override->lapic_address, 
				VMM_PAGE_P | VMM_PAGE_RW | VMM_PAGE_PCD | VMM_PAGE_PTE_PAT	/* Disable caching, IO memory shouldnt be cached. */
			);
			if((virt_addr_t)s_lapic_address_override == (virt_addr_t)-1)
			{
				status = ERR_OUT_OF_MEMORY;
				goto cleanup;
			}

			break;
		}
		
		default:
			break;
		}
	}

	status = lapic_init();

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

int apic_map_irq(uint8_t irq, uint8_t interrupt)
{
	ioapic_descriptor_t* ioapic = s_ioapic_descriptor;
	while(ioapic)
	{
		if(ioapic_irq_in_range(ioapic, irq))
			return ioapic_map_irq(ioapic, irq, interrupt);
		
		ioapic = ioapic->next;
	}

	return ERR_IRQ_NOT_SUPPORTED;
}

int ioapic_init(acpi_madt_record_ioapic_t* ioapic_record)
{
	if(!ioapic_record)
		return ERR_INVALID_PARAMETER;
	
	ioapic_descriptor_t* descriptor = (ioapic_descriptor_t*)malloc(sizeof(ioapic_descriptor_t));
	if(!descriptor)
		return ERR_OUT_OF_MEMORY;

	phys_addr_t ioapic_phys_base = (phys_addr_t)ioapic_record->ioapic_address;
	descriptor->first_irq = ioapic_record->global_system_interrupt_base;

	/* 
	 * Map the configuration space of the IO APIC. If already mapped, use its virtual address. 
	 * The configuration space of an IO APIC is guaranteed to be 4KiB aligned, see intel specification.
	 */
	virt_addr_t page = vmm_get_virtual_of(ioapic_phys_base);
	if(page == (virt_addr_t)-1)
		/* It is recommended to disable caching on IO APIC configuration space, so do it. */
		descriptor->mmio = (uint8_t*)vmm_map_physical_page(
			ioapic_phys_base, 
			VMM_PAGE_P | VMM_PAGE_RW | VMM_PAGE_PCD | VMM_PAGE_PTE_PAT
		);
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

int ioapic_map_irq(const ioapic_descriptor_t* ioapic, uint8_t irq, uint8_t interrupt)
{
	if(!ioapic || !ioapic_irq_in_range(ioapic, irq))
		return ERR_INVALID_PARAMETER;

	uint8_t ioapic_irq = irq - ioapic->first_irq;

	/* 
	 * TODO: When making multiprocessing and all of that, 
	 * find the core with least interrupts and map the interrupt to it. 
	 */
	uint8_t local_apic_id = lapic_alloc();

	uint32_t irq_redtbl_reg = APIC_IOAPIC_REG_IOREDTBL(ioapic_irq);

	ioapic_redtbl_entry_t entry = {
		.interrupt 			= interrupt,
		.delivery_mode 		= APIC_IOAPIC_REDTBL_DELIVERY_MODE_FIXED,
		.destination_mode 	= APIC_IOAPIC_REDTBL_DESTINATION_MODE_PHYSICAL,
		.delivery_status 	= 0,
		.pin_polarity		= APIC_IOAPIC_REDTBL_PIN_POLARITY_ACTIVE_HIGH,
		.remote_irr			= 0,
		.trigger_mode		= APIC_IOAPIC_REDTBL_TRIGGER_MODE_EDGE,
		.mask				= 0,
		.reserved			= 0,
		.destination		= local_apic_id
	};

	ioapic_write64(ioapic, irq_redtbl_reg, *(uint64_t*)&entry);		/* Cant cast directly to uint64_t, so do that. */

	return SUCCESS;
}

bool ioapic_irq_in_range(const ioapic_descriptor_t* ioapic, uint8_t irq)
{
	if(!ioapic)
		return false;

	uint32_t version_reg = ioapic_read32(ioapic, APIC_IOAPIC_REG_IOAPICVER);
	uint32_t max_redtbl = APIC_IOAPIC_IOAPICVER_MAX_REDTBL(version_reg);
	uint8_t last_irq = ioapic->first_irq + APIC_IOAPIC_REDTBL_TO_IRQ(max_redtbl);

	return irq > ioapic->first_irq && irq <= last_irq;
}

uint32_t ioapic_read32(const ioapic_descriptor_t* ioapic, uint8_t reg)
{
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg;
	return *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
}

uint64_t ioapic_read64(const ioapic_descriptor_t* ioapic, uint8_t reg)
{
	uint32_t low, high;
	
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg;
	low = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);

	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg + 1;
	high = *(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN);
	
	return (uint64_t)low | ((uint64_t)high << 32);
}

void ioapic_write32(const ioapic_descriptor_t* ioapic, uint8_t reg, uint32_t value)
{
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg;
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN) = value;
}

void ioapic_write64(const ioapic_descriptor_t* ioapic, uint8_t reg, uint64_t value)
{
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg;
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN) = (uint32_t)value;

	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGSEL) = (uint32_t)reg + 1;
	*(uint32_t volatile*)(ioapic->mmio + APIC_IOAPIC_IOREGWIN) = (uint32_t)(value >> 32);
}

int lapic_init()
{
	if(s_lapic_address_override == (uint64_t)-1)
	{
		/* Only bits 12-63 are used for the physical address of the mmio. */
		phys_addr_t phys = cpu_read_msr(MSR_IA32_APIC_BASE) & ~(phys_addr_t)(4096 - 1); 

		/* The same physical address might be used on multiple CPU's, so only if it is not mapped, map it. */
		if(vmm_get_virtual_of(phys) == (virt_addr_t)-1)
		{
			virt_addr_t virt = vmm_map_physical_page(
				phys,
				VMM_PAGE_P | VMM_PAGE_RW | VMM_PAGE_PCD | VMM_PAGE_PTE_PAT
			);
	
			if(virt == (virt_addr_t)-1)
				return ERR_OUT_OF_MEMORY;
		}
	}

	/* Spurious interrupt = 0xFF, set enable local apic bit (0x100) */
	uint32_t old_spurious = lapic_read_reg(LAPIC_REG_SPURIOUS_INT_VECTOR);
	lapic_write_reg(LAPIC_REG_SPURIOUS_INT_VECTOR, old_spurious | 0xFF | 0x100);	

	return SUCCESS;
}

uint32_t lapic_alloc()
{
	/* For now, all interrupts shall go to the BSP (Boot Strap Processor, basicaly the default one) */
	return lapic_read_reg(LAPIC_REG_ID);
}

uint64_t lapic_get_mmio()
{
	if(s_lapic_address_override == (uint64_t)-1)
	{
		/* Only bits 12-63 are used for the physical address of the mmio. */
		phys_addr_t phys = cpu_read_msr(MSR_IA32_APIC_BASE) & ~(phys_addr_t)(4096 - 1); 
		virt_addr_t virt = vmm_get_virtual_of(phys);
		if(virt == (virt_addr_t)-1)
			return (uint64_t)-1;

		return virt;
	}
		
	return s_lapic_address_override;
}

uint32_t lapic_read_reg(uint16_t reg)
{
	uint8_t* mmio = (uint8_t*)lapic_get_mmio();
	if(mmio)
		return *(uint32_t*)(mmio + (uint64_t)reg);
	
	return 0;
}

void lapic_write_reg(uint16_t reg, uint32_t value)
{
	uint8_t* mmio = (uint8_t*)lapic_get_mmio();
	if(mmio)
		*(uint32_t*)(mmio + (uint64_t)reg) = value; 
}