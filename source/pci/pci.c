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

#include "pci/pci.h"
#include "pci/device.h"

#include <stdlib.h>
#include "error.h"
#include "mm/pmm/pmm.h"
#include "mm/vmm/vmm.h"
#include "acpi/acpi.h"
#include "cpu.h"
#include "nvme/nvme.h"

static pci_access_mechanism_t s_pci_access_mechanism = (pci_access_mechanism_t)-1;
static uint8_t* s_pci_mmconfig = NULL;

int pci_init()
{
	uint8_t start_bus = 0;
	acpi_mcfg_t* mcfg = (acpi_mcfg_t*)acpi_find_table_copy(ACPI_MCFG_SIGNATURE);
	if(mcfg)
	{
		s_pci_access_mechanism = PCI_ACCESS_MMCONFIG;

		acpi_mcfg_config_t* mcfg_config = &mcfg->configurations[0];
		phys_addr_t base_address = mcfg_config->base_address;
		int buses = (int)(mcfg_config->end_bus_number - mcfg_config->start_bus_number + 1);
		size_t mmconfig_size = (size_t)buses * PCI_DEVICES_PER_BUS * PCI_FUNCTIONS_PER_DEVICE * 4096;
		start_bus = mcfg_config->start_bus_number;
		free(mcfg);
	
		s_pci_mmconfig = (uint8_t*)vmm_map_physical_pages(
			base_address, 
			VMM_PAGE_P | VMM_PAGE_RW,
			mmconfig_size / VMM_PAGE_SIZE
		);
	
		if(s_pci_mmconfig == (uint8_t*)-1)
			return ERR_OUT_OF_MEMORY;
	}
	else
		s_pci_access_mechanism = PCI_ACCESS_MECHANISM1;

	pci_enumerate_bus(start_bus, &g_device_root);
	return SUCCESS;
}

void pci_enumerate_bus(uint8_t bus, device_t* parent)
{
	for(uint8_t device = 0; device < PCI_DEVICES_PER_BUS; device++)
	{
		for(uint8_t function = 0; function < PCI_FUNCTIONS_PER_DEVICE; function++)
		{
			uint16_t vendor_id = pci_read16(bus, device, function, offsetof(pci_config_t, vendor_id));
			if(vendor_id == 0xFFFF)
				continue;
			
			device_pci_t* pci_device = pci_create_device(bus, device, function);
			if(pci_device)
			{
				parent->add_child(pci_device);
				int status = pci_device->initialize();
				if(status != SUCCESS)
					parent->remove_child(pci_device);
			}
			
			/* If its not a multi-function device, continue to the next device and dont enumerate functions for this device. */
			uint8_t header_type = pci_read8(bus, device, function, offsetof(pci_config_t, header_type));
			if(function == 0 && (header_type & (1 << 7)) == 0)		
				break;
		}
	}
}

device_pci_t* pci_create_device(uint8_t bus, uint8_t device, uint8_t function)
{
	uint8_t header_type = pci_read8(bus, device, function, offsetof(pci_config_t, header_type));
	uint8_t class_code = pci_read8(bus, device, function, offsetof(pci_config_t, class_code));
	uint8_t subclass = pci_read8(bus, device, function, offsetof(pci_config_t, subclass));
	// uint8_t prog_if = pci_read8(bus, device, function, offsetof(pci_config_t, prog_if));

	if((header_type & 1) == 1 || (class_code == 6 && subclass == 4 ))
		return new device_pci_bridge_pci2pci_t(bus, device, function);

	switch(class_code)
	{
	case PCI_CLASSCODE_MASS_STORAGE:
	{
		switch(subclass)
		{
		case PCI_SUBCLASS_NVM:
			return new device_storage_pci_nvme_t(bus, device, function);

		default:
			break;
		}

		break;
	}
	
	default:
		break;
	}

	return NULL; 
}

uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		return *(uint32_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset));
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		if(IS_ALIGNED(offset, 4))
			return pci_read_mechanism1(bus, device, function, offset);

		uint32_t low = pci_read_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4)) >> ((offset % 4) * 8);
		uint32_t high = pci_read_mechanism1(bus, device, function, ALIGN_UP(offset, 4)) << ((4 - offset % 4) * 8);
		return low | high;
	}
	
	return -1;
}

uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		return *(uint16_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset));
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		if(offset % 4 <= 2)
			return pci_read_mechanism1(bus, device, function, offset) >> ((offset % 4) * 8);

		uint16_t low = pci_read_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4)) >> (3*8);
		uint16_t high = pci_read_mechanism1(bus, device, function, ALIGN_UP(offset, 4)) << 8;
		return low | high;
	}
	
	return -1;
}

uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		return *(uint8_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset));
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		return pci_read_mechanism1(bus, device, function, offset) >> ((offset % 4) * 8);
	}
	
	return -1;
}

void pci_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		*(uint32_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset)) = value;
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		if(IS_ALIGNED(offset, 4))
			pci_write_mechanism1(bus, device, function, offset, value);
		else
		{
			uint32_t low_mask = 0xFFFFFFFF << ((offset % 4) * 8);
			uint32_t low_old = pci_read_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4));
			uint32_t low_new = (low_old & ~low_mask) | (value << ((offset % 4) * 8));
			pci_write_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4), low_new);

			uint32_t high_mask = 0xFFFFFFFF >> ((4 - offset % 4) * 8);
			uint32_t high_old = pci_read_mechanism1(bus, device, function, ALIGN_UP(offset, 4));
			uint32_t high_new = (high_old & ~high_mask) | (value >> ((4 - offset % 4) * 8));
			pci_write_mechanism1(bus, device, function, ALIGN_UP(offset, 4), high_new);
		}
	}
}

void pci_write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		*(uint16_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset)) = value;
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		if(offset % 4 <= 2)
		{
			uint32_t mask = 0xFFFF << ((offset % 4) * 8);
			uint32_t old = pci_read_mechanism1(bus, device, function, offset);
			uint32_t new_value = (old & ~mask) | ((uint32_t)value << ((offset % 4) * 8));
			pci_write_mechanism1(bus, device, function, offset, new_value);
		}
		else
		{
			uint32_t low_old = pci_read_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4));
			uint32_t low_new = (low_old & 0x00FFFFFF) | ((uint32_t)value << (3*8));
			pci_write_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4), low_new);

			uint32_t high_old = pci_read_mechanism1(bus, device, function, ALIGN_UP(offset, 4));
			uint32_t high_new = (high_old & 0xFFFFFF00) | ((uint32_t)value >> 8);
			pci_write_mechanism1(bus, device, function, ALIGN_UP(offset, 4), high_new);
		}
	}
}

void pci_write8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		*(uint8_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset)) = value;
	}
	else if(s_pci_access_mechanism == PCI_ACCESS_MECHANISM1)
	{
		uint32_t mask = 0xFF << ((offset % 4) * 8);
		uint32_t old = pci_read_mechanism1(bus, device, function, offset);
		uint32_t new_value = (old & ~mask) | ((uint32_t)value << ((offset % 4) * 8));
		pci_write_mechanism1(bus, device, function, offset, new_value);
	}
}

uint32_t pci_read_mechanism1(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t address = PCI_MECHANISM1_ADDRESS(bus, device, function, offset);
	outl(PCI_CONFIG_PORT, address);
	return inl(PCI_DATA_PORT);
}

void pci_write_mechanism1(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	uint32_t address = PCI_MECHANISM1_ADDRESS(bus, device, function, offset);
	outl(PCI_CONFIG_PORT, address);
	outl(PCI_DATA_PORT, value);
}