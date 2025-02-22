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

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"

typedef struct acpi_rsdp 
{
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct acpi_xsdp 
{
	acpi_rsdp_t rsdp;
	uint32_t size;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed)) acpi_xsdp_t;

typedef struct acpi_sdt_header 
{
	char signature[4];
	uint32_t size;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

typedef struct acpi_rsdt 
{
	acpi_sdt_header_t header;
	uint32_t sdt_pointers[];				/* An array of 32bit physical addresses, each address points to some SDT. */
} __attribute__((packed)) acpi_rsdt_t;

typedef struct acpi_xsdt 
{
	acpi_sdt_header_t header;
	uint64_t sdt_pointers[]; 				/* An array of 64bit physical addresses, each address points to some SDT. */
} __attribute__((packed)) acpi_xsdt_t;

typedef struct acpi_mcfg 
{
	acpi_sdt_header_t header;
	uint8_t reserved[8];
	struct 
	{
		uint64_t base_address;
		uint16_t segment_group_number;
		uint8_t start_bus_number;
		uint8_t end_bus_number;
		uint8_t reserved[4];
	} __attribute__((packed)) configurations[];
} __attribute__((packed)) acpi_mcfg_t;