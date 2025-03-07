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
	return SUCCESS;
}