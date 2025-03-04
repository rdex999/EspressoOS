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
#include "pci/pci.h"

class device_pci_t : public device_t
{
public:
	device_pci_t(device_type_t type, uint8_t bus, uint8_t device, uint8_t function)
		: ::device_t(type | DEVICE_TYPE_PCI), m_bus(bus), m_device(device), m_function(function) {}

	virtual int initialize() override;

protected:
	virtual bool is_device(const device_t* dev) const override;

	const uint8_t m_bus;
	const uint8_t m_device;
	const uint8_t m_function;

	uint16_t m_vendor_id;
	uint16_t m_device_id;
	uint8_t m_class_code;
	uint8_t m_subclass;
};

class device_pci_bridge_pci2pci_t : public device_pci_t
{
public:
	device_pci_bridge_pci2pci_t(uint8_t bus, uint8_t device, uint8_t function)
		: device_pci_t(DEVICE_TYPE_PCI_BRIDGE, bus, device, function) {}

	int initialize() override;
	int uninitialize() override { return SUCCESS; };

protected:
	void discover_children() override;
};