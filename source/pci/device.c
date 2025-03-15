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

#include <stdint.h>
#include <stddef.h>
#include "mm/vmm/vmm.h"
#include "common.h"

int device_pci_t::initialize()
{
	m_vendor_id = pci_read16(m_bus, m_device, m_function, offsetof(pci_config_t, vendor_id));
	m_device_id = pci_read16(m_bus, m_device, m_function, offsetof(pci_config_t, device_id));
	m_class_code = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, class_code));
	m_subclass = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, subclass));
	m_prog_if = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, prog_if));

	return SUCCESS;
}

bool device_pci_t::is_device(const device_t* device) const
{
	/* 
	 * Okay so if class code if specified (not -1), check if it matches. 
	 * If doesnt matche, return false. If it matches, check if subclass is specified (not -1).
	 * if not specified, return true. If specified, check if it matches. If it doesnt, return false, else return true.
	 * And so on, check from the least specific to the most specific identifiers.
	 */

	/* If its not even a PCI device, return false. */
	if((device->m_type & DEVICE_TYPE_PCI) == 0)
		return false;
	
	const device_pci_t* pci_device = (const device_pci_t*)(device->m_self);

	if(pci_device->m_class_code != m_class_code)
		return false;

	/* User searched only for class code */
	if(pci_device->m_subclass == (uint8_t)-1)
		return true;

	if(pci_device->m_subclass != m_subclass)
		return false;

	/* User searched only for prog if */	
	if(pci_device->m_prog_if == (uint8_t)-1)
		return true;

	if(pci_device->m_prog_if != m_prog_if)
		return false;

	/* User searched only for class code, subclass */
	if(pci_device->m_vendor_id == (uint16_t)-1)
		return true;
	
	if(pci_device->m_vendor_id != m_vendor_id)
		return false;
	
	/* User searched for class code, subclass, device id */
	if(pci_device->m_device_id == (uint16_t)-1)
		return true;

	/* User made a specific search on all identifiers. */
	return pci_device->m_device_id == m_device_id;
}

int device_pci_t::msix_init(uint16_t msi_capability)
{
	if(msi_capability == (uint16_t)-1)
		return ERR_INVALID_PARAMETER;

	uint16_t message_control = pci_read16(
		m_bus, 
		m_device, 
		m_function, 
		msi_capability + offsetof(pci_capability_msix_t, message_control)
	);

	message_control |= PCI_MSIX_REG_CTRL_ENABLE;		/* Enable MSI-X */
	message_control |= PCI_MSIX_REG_CTRL_MASK;		/* Mask (disable) all interrupts. */

	pci_write16(
		m_bus, 
		m_device, 
		m_function, 
		msi_capability + offsetof(pci_capability_msix_t, message_control), 
		message_control
	);

	uint32_t table_desc = pci_read32(
		m_bus, 
		m_device, 
		m_function, 
		msi_capability + offsetof(pci_capability_msix_t, table_descriptor)
	);
	uint8_t table_bar_index = PCI_MSIX_REG_BAR_ADDR_BAR_IDX(table_desc);
	uint32_t table_offset = PCI_MSIX_REG_BAR_ADDR_OFFSET(table_desc);
	size_t table_pages = DIV_ROUND_UP(
		table_offset + PCI_MSIX_REG_CTRL_GET_TABLE_LENGTH(message_control) * sizeof(pci_msix_table_entry_t), 
		VMM_PAGE_SIZE
	);

	uint32_t pending_desc = pci_read32(
		m_bus, 
		m_device, 
		m_function, 
		msi_capability + offsetof(pci_capability_msix_t, pending_descriptor)
	);
	uint8_t pending_bar_index = PCI_MSIX_REG_BAR_ADDR_BAR_IDX(pending_desc);
	uint32_t pending_offset = PCI_MSIX_REG_BAR_ADDR_OFFSET(pending_desc);
	size_t pending_pages = DIV_ROUND_UP(
		pending_offset + DIV_ROUND_UP(PCI_MSIX_REG_CTRL_GET_TABLE_LENGTH(message_control), 8), 	/* Amount of bytes for the bitmap */
		VMM_PAGE_SIZE
	);	

	if(pending_bar_index == table_bar_index)
	{
		size_t pages = MAX(table_pages, pending_pages);
		void* mapped_bar = map_bar(table_bar_index, pages);
		if(mapped_bar == (void*)-1)
			return ERR_OUT_OF_MEMORY;
		
		m_msix_table = (pci_msix_table_entry_t*)((uint8_t*)mapped_bar + (uint64_t)table_offset);
		m_msix_pending = (pci_msix_pending_entry_t*)((uint8_t*)mapped_bar + (uint64_t)pending_offset);
	}
	else
	{
		void* mapped_table_bar = map_bar(table_bar_index, table_pages);
		if(mapped_table_bar == (void*)-1)
			return ERR_OUT_OF_MEMORY;

		void* mapped_pending_bar = map_bar(pending_bar_index, pending_pages);
		if(mapped_pending_bar == (void*)-1)
			return ERR_OUT_OF_MEMORY;

		m_msix_table = (pci_msix_table_entry_t*)((uint8_t*)mapped_table_bar + (uint64_t)table_offset);
		m_msix_pending = (pci_msix_pending_entry_t*)((uint8_t*)mapped_pending_bar + (uint64_t)pending_offset);
	}
	
	return SUCCESS;
}

void* device_pci_t::map_bar(uint8_t bar, size_t pages) const
{
	uint32_t low = pci_read32(
		m_bus, 
		m_device, 
		m_function, 
		offsetof(pci_config_t, device.base_address[0]) + bar * sizeof(uint32_t)
	);

	uint64_t flags = VMM_PAGE_P | VMM_PAGE_RW;
	phys_addr_t physical = (uint64_t)low & ~0xF;

	if(PCI_BAR_GET_TYPE(low) == PCI_BAR_TYPE_64BIT)
	{
		uint32_t high = pci_read32(
			m_bus, 
			m_device, 
			m_function, 
			offsetof(pci_config_t, device.base_address[0]) + (bar + 1) * sizeof(uint32_t)
		);

		physical |= (uint64_t)high << 32;
	}

	/* The osdev wiki states that if a page is prefetchable, paging should mark it as write-through for maximum performance. */
	if(PCI_BAR_GET_PREFETCHABLE(low) == 1)
		flags |= VMM_PAGE_PWT;
	else
		flags |= VMM_PAGE_PCD | VMM_PAGE_PTE_PAT;

	return (void*)vmm_map_physical_pages(
		physical, 
		flags, 
		pages
	);
}

uint16_t device_pci_t::find_capability(pci_capability_id_t capability) const
{
	int reg_status = pci_read16(m_bus, m_device, m_function, offsetof(pci_config_t, status));
	if((reg_status & PCI_STATUS_CAPABILITIES) == 0)
		return -1;
	
	uint8_t current = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, device.capabilities_pointer));
	while(current)
	{
		uint8_t id = pci_read8(m_bus, m_device, m_function, (uint16_t)current + offsetof(pci_capability_header_t, id));
		if(id == (uint8_t)capability)
			return (uint16_t)current;
		
		current = pci_read8(m_bus, m_device, m_function, (uint16_t)current + offsetof(pci_capability_header_t, next));
	}

	return -1;
}

int device_pci_bridge_pci2pci_t::initialize()
{
	static int next_free_bus = 1;

	int status = device_pci_t::initialize();
	if(status != SUCCESS)
		return status;

	uint8_t secondary_bus = next_free_bus++;
	uint8_t subordinate_bus = secondary_bus;

	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_t, bridge_pci_to_pci.primary_bus), m_bus);
	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_t, bridge_pci_to_pci.secondary_bus), secondary_bus);
	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_t, bridge_pci_to_pci.subordinate_bus), subordinate_bus);

	discover_children();

	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_t, bridge_pci_to_pci.subordinate_bus), next_free_bus - 1);

	return SUCCESS;
}

void device_pci_bridge_pci2pci_t::discover_children()
{
	uint8_t secondary_bus = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, bridge_pci_to_pci.secondary_bus));
	pci_enumerate_bus(secondary_bus, this);
}