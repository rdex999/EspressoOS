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

#include "device/device.h"
#include "storage/storage.h"
#include "pci/pci.h"

class device_storage_pci_nvme_t : public device_storage_t, public device_pci_t
{
public:	
	device_storage_pci_nvme_t(uint8_t bus, uint8_t device, uint8_t function) : 
		device_t(DEVICE_TYPE_STORAGE | DEVICE_TYPE_PCI | DEVICE_TYPE_NVME, this),
		device_storage_t(),
		device_pci_t(DEVICE_TYPE_STORAGE | DEVICE_TYPE_PCI | DEVICE_TYPE_NVME, bus, device, function) {}

	int initialize() override;
	int uninitialize() override;

	void discover_children() override {};
	
protected:
	bool is_device(const device_t* device) const override;
	int read_sectors(uint64_t lba, size_t count, void* buffer) const override;
	int write_sectors(uint64_t lba, size_t count, const void* buffer) const override;
};