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
static pci_config_t* s_pci_mmconfig = NULL;

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
	
		s_pci_mmconfig = (pci_config_t*)vmm_map_physical_pages(
			base_address, 
			VMM_PAGE_P | VMM_PAGE_RW,
			mmconfig_size / VMM_PAGE_SIZE
		);
	
		if(s_pci_mmconfig == (pci_config_t*)-1)
			return ERR_OUT_OF_MEMORY;
	}
	else
		s_pci_access_mechanism = PCI_ACCESS_MECHANISM1;

	return SUCCESS;
}

uint32_t pci_read_mechanism1(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset)
{
	uint32_t address = (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | (1 << 31);
	outl(PCI_CONFIG__PORT, address);
	return inl(PCI_DATA_PORT);
}

void pci_write_mechanism1(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value)
{
	uint32_t address = (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC) | (1 << 31);
	outl(PCI_CONFIG__PORT, address);
	outl(PCI_DATA_PORT, value);
}