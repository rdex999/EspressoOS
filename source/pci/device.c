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

#include <stdint.h>
#include <stddef.h>
#include "pci/pci.h"

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