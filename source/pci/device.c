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
#include "pci/device.h"

void device_pci::initialize()
{
	m_vendor_id = pci_read16(m_bus, m_device, m_function, offsetof(pci_config_t, vendor_id));
	m_device_id = pci_read16(m_bus, m_device, m_function, offsetof(pci_config_t, device_id));
	m_class_code = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, class_code));
	m_subclass = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_t, subclass));
}

bool device_pci::is_device(const device* dev) const
{
	/* 
	 * Okay so if class code if specified (not -1), check if it matches. 
	 * If doesnt matche, return false. If it matches, check if subclass is specified (not -1).
	 * if not specified, return true. If specified, check if it matches. If it doesnt, return false, else return true.
	 * And so on, check from the least specific to the most specific identifiers.
	 */

	/* If its not even a PCI device, return false. */
	if((dev->m_type & DEVICE_TYPE_PCI) == 0)
		return false;
	
	const device_pci* pci_device = (const device_pci*)dev;
	if(pci_device->m_class_code != m_class_code)
		return false;

	/* User searched only for class code */
	if(pci_device->m_subclass == (uint8_t)-1)
		return true;

	if(pci_device->m_subclass != m_subclass)
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

void device_pci_bridge::initialize()
{
	device_pci::initialize();

	/* For some reason, we need (should) to read the bus numbers and write them back. */
	uint8_t secondary_bus = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_bridge_t, secondary_bus));
	uint8_t subordinate_bus = pci_read8(m_bus, m_device, m_function, offsetof(pci_config_bridge_t, subordinate_bus));

	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_bridge_t, primary_bus), m_bus);
	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_bridge_t, secondary_bus), secondary_bus);
	pci_write8(m_bus, m_device, m_function, offsetof(pci_config_bridge_t, subordinate_bus), subordinate_bus);
}