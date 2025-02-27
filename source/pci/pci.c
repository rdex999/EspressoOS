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

static pci_access_mechanism_t s_pci_access_mechanism = (pci_access_mechanism_t)-1;
static uint8_t* s_pci_mmconfig = NULL;

int pci_init()
{
	acpi_mcfg_t* mcfg = (acpi_mcfg_t*)acpi_find_table_copy(ACPI_MCFG_SIGNATURE);
	if(mcfg)
	{
		s_pci_access_mechanism = PCI_ACCESS_MMCONFIG;

		acpi_mcfg_config_t* mcfg_config = &mcfg->configurations[0];
		phys_addr_t base_address = mcfg_config->base_address;
		int buses = (int)(mcfg_config->end_bus_number - mcfg_config->start_bus_number + 1);
		size_t mmconfig_size = (size_t)buses * PCI_DEVICES_PER_BUS * PCI_FUNCTIONS_PER_DEVICE * 4096;

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

	return SUCCESS;
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

		uint16_t low = pci_read_mechanism1(bus, device, function, ALIGN_DOWN(offset, 4)) >> ((offset % 4) * 8);
		uint16_t high = pci_read_mechanism1(bus, device, function, ALIGN_UP(offset, 4)) << ((4 - offset % 4) * 8);
		return low | high;
	}
	
	return -1;
}

uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset)
{
	if(s_pci_access_mechanism == PCI_ACCESS_MMCONFIG)
	{
		return *(uint16_t*)(s_pci_mmconfig + PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset));
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