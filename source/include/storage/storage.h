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
#include <stdlib.h>
#include <string.h>
#include "device/device.h"
#include "pci/pci.h"

class device_storage_t : public device_t
{
public:
	device_storage_t()
		: device_t(DEVICE_TYPE_STORAGE) {}

	virtual bool is_device(const device_t* device) const override;

	/* 
	 * Read <size> bytes on offset <offset> from the sector at <lba> into <buffer>. 
	 * Returns 0 on success, an error code otherwise. 
	 * WARNING: Untested
	 */
	int read(uint64_t lba, int offset, size_t size, void* buffer) const;

	/* 
	 * Write <size> bytes on offset <offset> to the sector at <lba> from <buffer>.
	 * Returns 0 on success, an error code otherwise. 
	 * WARNING: Untested
	 */
	int write(uint64_t lba, int offset, size_t size, const void* buffer) const;

protected:
	/* 
	 * Read <count> sectors starting from <lba>, data goes into <buffer>
	 * Returns 0 on success, an error code otherwise.
	 */	
	virtual int read_sectors(uint64_t lba, int count, void* buffer) const = 0;

	/* 
	 * Write <count> sectors into <lba> from <buffer>.
	 * Returns 0 on success, an error code otherwise.
	 */	
	virtual int write_sectors(uint64_t lba, int count, const void* buffer) const = 0;

	int m_sector_size;
};