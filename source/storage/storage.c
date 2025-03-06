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

#include "storage/storage.h"

bool device_storage_t::is_device(const device_t* device) const
{
	if(!device)
		return false;
	
	if((device->m_type & DEVICE_TYPE_STORAGE) == 0)
		return false;
	
	return true;
}

int device_storage_t::read(uint64_t lba, int offset, size_t size, void* buffer) const
{
	if(!buffer || size == (size_t)0)
		return ERR_INVALID_PARAMETER;
	
	lba += offset / m_sector_size;						/* If offset is greater than the size of a sector, update the LBA */
	offset -= (offset / m_sector_size) * m_sector_size;

	int status;	
	uint8_t* destination = (uint8_t*)buffer;
	size_t bytes_left = size;
	uint8_t* sector_buffer = NULL;
	uint64_t current_lba = lba;

	/* 
	 * If doing an unaligned (by sector size) read, or a read thats less then the size of a sector,
	 * read the first sector into a temporary buffer, and do an offseted copy into <destination>.
	 */
	if(offset != 0 || bytes_left < (size_t)m_sector_size - (size_t)offset)
	{
		sector_buffer = (uint8_t*)malloc((size_t)m_sector_size);
		if(!sector_buffer)
			return ERR_OUT_OF_MEMORY;
		
		status = read_sectors(current_lba, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
	
		size_t read_size;
		if(bytes_left < (size_t)m_sector_size - (size_t)offset)
		{
			read_size = bytes_left;
		}
		else
		{
			read_size = (size_t)m_sector_size - (size_t)offset;
			++current_lba;
		}
		
		memcpy(destination, sector_buffer + (uint64_t)offset, read_size);
		destination += (uint64_t)read_size;
		bytes_left -= (size_t)read_size;
	}

	/* Check if we can read full sectors straight into <buffer> (<destination>) */
	if(bytes_left >= (size_t)m_sector_size)
	{
		int sectors = bytes_left / m_sector_size;
		size_t read_size = (uint64_t)sectors * (uint64_t)m_sector_size;

		status = read_sectors(current_lba, sectors, destination);
		if(status != SUCCESS)
			goto cleanup;

		destination += read_size;
		bytes_left -= read_size;
		current_lba += (uint64_t)sectors;
	}

	/* 
	 * Here we know that <bytes_left> is less than a sector. 
	 * If its not 0, then read into the temporary buffer and then copy the left over bytes into <destination>.
	 */
	if(bytes_left != 0)
	{
		if(!sector_buffer)
		{
			sector_buffer = (uint8_t*)malloc((size_t)m_sector_size);
			if(!sector_buffer)
				return ERR_OUT_OF_MEMORY;
		}

		status = read_sectors(current_lba, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
		
		memcpy(destination, sector_buffer, bytes_left);	
	}

	status = SUCCESS;
cleanup:
	if(sector_buffer)
		free(sector_buffer);

	return status;
}

int device_storage_t::write(uint64_t lba, int offset, size_t size, const void* buffer) const
{
	if(!buffer || size == (size_t)0)
		return ERR_INVALID_PARAMETER;
	
	lba += (size_t)offset / (size_t)m_sector_size;		/* If offset is greater than the size of a sector, update the LBA */
	offset -= (offset / m_sector_size) * m_sector_size;

	int status;
	uint8_t* sector_buffer = NULL;
	const uint8_t* source = (const uint8_t*)buffer;
	uint64_t current_lba = lba;
	size_t bytes_left = size;

	/* 
	 * If writing unaligned (by sector size), or write of size less then the size of a sector,
	 * read the first sector (<lba>), update the needed bytes on <offset>, and write the sector back to disk.
	 */
	if(offset != 0 || size < (size_t)m_sector_size - (size_t)offset)
	{
		sector_buffer = (uint8_t*)malloc((size_t)m_sector_size);
		if(!sector_buffer)
			return ERR_OUT_OF_MEMORY;
		
		status = read_sectors(current_lba, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
		
		size_t write_size;
		if(bytes_left < (size_t)m_sector_size - (size_t)offset)
			write_size = bytes_left;
		else
			write_size = (size_t)m_sector_size - (size_t)offset;

		memcpy(sector_buffer + (uint64_t)offset, source, write_size);

		status = write_sectors(current_lba++, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
		
		bytes_left -= write_size;
		source += write_size;
	}

	/* Check if we can write full sectors from <source> into <current_lba> */
	if(bytes_left >= (size_t)m_sector_size)
	{
		int sectors = bytes_left / (size_t)m_sector_size;
		size_t write_size = sectors * (size_t)m_sector_size;
		status = write_sectors(current_lba, sectors, source);
		if(status != SUCCESS)
			goto cleanup;
		
		source += write_size;
		current_lba += (uint64_t)sectors;
		bytes_left -= write_size;
	}

	/* 
	 * If there are still some bytes to write (and we know its less then the size of a sector), 
	 * read the sector, update the needed bytes, and write it back to disk.
	 */
	if(bytes_left != (size_t)0)
	{
		if(!sector_buffer)
		{
			sector_buffer = (uint8_t*)malloc((size_t)m_sector_size);
			if(!sector_buffer)
				return ERR_OUT_OF_MEMORY;
		}

		status = read_sectors(current_lba, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
		
		memcpy(sector_buffer, source, bytes_left);
		
		status = write_sectors(current_lba, 1, sector_buffer);
		if(status != SUCCESS)
			goto cleanup;
	}

	status = SUCCESS;

cleanup:
	if(sector_buffer)
		free(sector_buffer);

	return status;
}