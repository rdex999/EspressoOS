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

#include "nvme/nvme.h"

#include "error.h"

int device_storage_pci_nvme_t::initialize()
{
	return SUCCESS;
}

int device_storage_pci_nvme_t::uninitialize()
{
	return SUCCESS;
}

int device_storage_pci_nvme_t::read_sectors(uint64_t lba, size_t count, void* buffer) const
{
	return SUCCESS;
}

int device_storage_pci_nvme_t::write_sectors(uint64_t lba, size_t count, const void* buffer) const
{
	return SUCCESS;
}