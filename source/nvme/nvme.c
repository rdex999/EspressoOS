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
#include "mm/vmm/vmm.h"

int device_storage_pci_nvme_t::initialize()
{
	int status;
	status = device_pci_t::initialize();
	if(status != SUCCESS)
		return status;
	
	m_mmio = (nvme_registers_t*)map_bar(0, 1);
	if(m_mmio == (void*)-1)
		return ERR_OUT_OF_MEMORY;

	status = msix_init();
	if(status != SUCCESS)
		return status;

	// status = msix_unmask_all();

	return SUCCESS;
}

int device_storage_pci_nvme_t::uninitialize()
{
	return SUCCESS;
}

int device_storage_pci_nvme_t::read_sectors(uint64_t, size_t, void*) const
{
	return SUCCESS;
}

int device_storage_pci_nvme_t::write_sectors(uint64_t, size_t, const void*) const
{
	return SUCCESS;
}