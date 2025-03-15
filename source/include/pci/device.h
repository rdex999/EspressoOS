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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "device/device.h"

typedef enum pci_capability_id
{
	PCI_CAPABILITY_ID_MSI 	= 0x05,
	PCI_CAPABILITY_ID_MSIX	= 0x11,
} pci_capability_id_t;

class device_pci_t : public virtual device_t
{
public:
	device_pci_t(device_type_t type, uint8_t bus, uint8_t device, uint8_t function)
		: device_t(type | DEVICE_TYPE_PCI, this), m_bus(bus), m_device(device), m_function(function) {}

	virtual int initialize() override;

protected:
	virtual bool is_device(const device_t* device) const override;

	/* 
	 * Map <pages> pages of a 64 bit Base Address Register (bar). Sets <flags> as the virtual address flags .
	 * Uses bar <bar> as the low 32 bits, and bar <bar>+1 as the high 32 bits of the physical address. 
	 * <bar> is the number of the bar (bar 0, bar 1, ...)
	 * Returns a valid pointer on success, -1 on failure.
	 */
	void* map_bar(uint8_t bar, size_t pages) const;

	/* 
	 * Find a capability of <capability> ID in the device's capabilities linked list.
	 * Returns an offset into the device's configuration space which has the capability.
	 * Returns -1 on failure. (Either the device doesnt support capabilities, or the given capability is not found.)
	 */
	uint16_t find_capability(pci_capability_id_t capability) const;

	/* 
	 * Initialize MSI-X for the device. Enable MSI and mask all interrupts.
	 * Returns 0 on success, an error code otherwise.
	 */
	int msix_init();

	/* Mask all interrupts for this device. Sets the mask bit in the MSIX-X control register. */
	void msix_mask_all() const;

	/* Unmask all interrupts for this device. Clears the mask bit in the MSIX-X control register. */
	void msix_unmask_all() const;

	const uint8_t m_bus;
	const uint8_t m_device;
	const uint8_t m_function;

	uint16_t m_vendor_id;
	uint16_t m_device_id;
	uint8_t m_class_code;
	uint8_t m_subclass;
	uint8_t m_prog_if;

	uint16_t m_msix_capability;
	struct pci_msix_table_entry* m_msix_table;
	uint64_t* m_msix_pending;
};

class device_pci_bridge_pci2pci_t : public device_pci_t
{
public:
	device_pci_bridge_pci2pci_t(uint8_t bus, uint8_t device, uint8_t function) : 
		device_t(DEVICE_TYPE_PCI_BRIDGE, this), 
		device_pci_t(DEVICE_TYPE_PCI_BRIDGE, bus, device, function) {}

	int initialize() override;
	int uninitialize() override { return SUCCESS; };

protected:
	void discover_children() override;
};