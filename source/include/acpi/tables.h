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
#define ACPI_MCFG_SIGNATURE "MCFG"
#define ACPI_MADT_SIGNATURE "APIC"

#define ACPI_MADT_TYPE_LOCAL_APIC 						0
#define ACPI_MADT_TYPE_IOAPIC 							1
#define ACPI_MADT_TYPE_INTERRUPT_SOURCE_OVERRIDE 		2
#define ACPI_MADT_TYPE_IOAPIC_NMI_SOURCE				3
#define ACPI_MADT_TYPE_LOCAL_APIC_NMI					4
#define ACPI_MADT_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE		5
#define ACPI_MADT_TYPE_PROCESSOR_LOCAL_X2APIC			9

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

typedef struct acpi_mcfg_config 
{
	uint64_t base_address;
	uint16_t segment_group_number;
	uint8_t start_bus_number;
	uint8_t end_bus_number;
	uint8_t reserved[4];
} __attribute__((packed)) acpi_mcfg_config_t;

typedef struct acpi_mcfg 
{
	acpi_sdt_header_t header;
	uint8_t reserved[8];
	acpi_mcfg_config_t configurations[];
} __attribute__((packed)) acpi_mcfg_t;

/* Structures for the MADT table, and its records. For more info, see: https://wiki.osdev.org/MADT */

typedef struct acpi_madt_record_header
{
	uint8_t type;
	uint8_t size;
} __attribute__((packed)) acpi_madt_record_header_t;

typedef struct acpi_madt_record_local_apic
{
	acpi_madt_record_header_t header;		/* type 0 */
	uint8_t acpi_processor_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed)) acpi_madt_record_local_apic_t;

typedef struct acpi_madt_record_ioapic
{
	acpi_madt_record_header_t header;		/* type 1 */
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_address;				/* The physical address of the IO APIC configuration space */
	uint32_t global_system_interrupt_base;	/* First interrupt that this IO APIC handles. */
} __attribute__((packed)) acpi_madt_record_ioapic_t;

typedef struct acpi_madt_record_interrupt_source_override
{
	acpi_madt_record_header_t header;		/* type 2 */
	uint8_t bus_source;
	uint8_t irq_source;
	uint16_t flags;
	uint32_t global_system_interrupt;
} __attribute__((packed)) acpi_madt_record_interrupt_source_override_t;

typedef struct acpi_madt_record_ioapic_nmi_source
{
	acpi_madt_record_header_t header; 		/* type 3 */
	uint8_t nmi_source;
	uint8_t reserved;
	uint16_t flags;
	uint32_t global_system_interrupt;
} __attribute__((packed)) acpi_madt_record_ioapic_nmi_source_t;

typedef struct acpi_madt_record_local_apic_nmi
{
	acpi_madt_record_header_t header;		/* type 4 */
	uint8_t processor_id;					/* 0xFF - all processors */
	uint16_t flags;
	uint8_t lint;							/* 0 or 1 */
} __attribute__((packed)) acpi_madt_record_local_apic_nmi_t;

typedef struct acpi_madt_record_local_apic_address_override
{
	acpi_madt_record_header_t header;		/* type 5 */
	uint16_t reserved;
	uint64_t local_apic_address;			/* Physical address of this local APIC. */
} __attribute__((packed)) acpi_madt_record_local_apic_address_override_t;

typedef struct acpi_madt_record_processor_local_x2apic
{
	acpi_madt_record_header_t header;		/* type 9 */
	uint16_t reserved;
	uint32_t local_x2apic_id;
	uint32_t flags;							/* Same aas local APIC flags */
	uint32_t acpi_id;
} __attribute__((packed)) acpi_madt_record_processor_local_x2apic_t;

typedef struct acpi_madt
{
	acpi_sdt_header_t header;
	uint32_t local_apic_address;
	uint32_t flags;
	acpi_madt_record_header_t records[];	/* Do not index into this array, use it as a pointer. (Structures of different sizes) */
} __attribute__((packed)) acpi_madt_t;