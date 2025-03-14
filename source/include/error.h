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

typedef enum error
{
	SUCCESS = 0,
	ERR_OUT_OF_MEMORY,
	ERR_PAGE_NOT_MAPPED,
	ERR_ACPI_NOT_INITIALIZED,
	ERR_ACPI_TABLE_NOT_FOUND,
	ERR_ACPI_RSDP_NOT_FOUND,
	ERR_ACPI_MADT_NOT_FOUND,
	ERR_APIC_NOT_SUPPORTED,
	ERR_ACPI_TABLE_CHECKSUM,
	ERR_INVALID_PARAMETER,
	ERR_IRQ_NOT_SUPPORTED,
	ERR_DEVICE_MSI_NOT_SUPPORTED,
	ERR_DEVICE_MSIX_NOT_SUPPORTED,
} error_t;