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
#include "pci/device.h"

#define PCI_DEVICES_PER_BUS 		32
#define PCI_FUNCTIONS_PER_DEVICE 	8
#define PCI_CONFIG_PORT 			0xCF8
#define PCI_DATA_PORT 				0xCFC

#define PCI_MMCONFIG_ADDRESS_OFFSET(bus, device, function, offset) \
	((((uint64_t)(bus) << 20) | ((uint64_t)(device) << 15) | ((uint64_t)(function) << 12)) + (uint64_t)(offset))

#define PCI_MECHANISM1_ADDRESS(bus, device, function, offset) \
	(((uint32_t)(bus) << 16) | ((uint32_t)(device) << 11) | ((uint32_t)(function) << 8) | ((uint32_t)(offset) & 0xFC) | (1 << 31))

#define PCI_STATUS_CAPABILITIES		(1 << 4)

#define PCI_BAR_GET_TYPE(bar)			(((bar) >> 1) & 0b11)
#define PCI_BAR_GET_PREFETCHABLE(bar)	(((bar) >> 3) & 0b1)

#define PCI_BAR_TYPE_64BIT			0b10
#define PCI_BAR_TYPE_32BIT			0b00

/* 
 * For information about MSI and that stuff see the PCI Local Bus 3.0 specification page 231 at:
 * https://lekensteyn.nl/files/docs/PCI_SPEV_V3_0.pdf
 */

#define PCI_MSI_REG_CONTROL_ENABLE 									(1 << 0)
#define PCI_MSI_REG_CONTROL_MULTIPLE_MSG_CAPABLE(control, count)	(1 << ((control) >> 1))				/* vectors=2**CAPABLE */
#define PCI_MSI_REG_CONTROL_MULTIPLE_MSG_ENABLE_SET(control, count) (((control) & ~0x70) | (log2(ALIGN_UP_POWER2(count)) << 4))	/* vectors=2**ENABLE */
#define PCI_MSI_REG_CONTROL_MULTIPLE_MSG_ENABLE_GET(control) 		(1 << ((control) >> 4))

#define PCI_MSIX_REG_CTRL_GET_TABLE_LENGTH(control)					(((control) & 0x7FF) + 1) /* Table length is 0 based, so add 1. */
#define PCI_MSIX_REG_CTRL_MASK										(1 << 14)
#define PCI_MSIX_REG_CTRL_ENABLE									(1 << 15)
#define PCI_MSIX_REG_BAR_ADDR_BAR_IDX(reg)							((reg) & 0b111)
#define PCI_MSIX_REG_BAR_ADDR_OFFSET(reg)							((reg) & ~0b111)

#define PCI_CLASSCODE_MASS_STORAGE 	1

#define PCI_SUBCLASS_NVM			8

#define PCI_PROGIF_NVME				2

typedef enum pci_access_mechanism
{
	PCI_ACCESS_MECHANISM1,
	PCI_ACCESS_MMCONFIG,
} pci_access_mechanism_t;

typedef struct pci_config_device
{
	uint32_t base_address[6];
	uint32_t cardbus_cis_pointer;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint32_t expansion_rom_base_address;
	uint8_t capabilities_pointer;
	uint8_t reserved[7];
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint8_t min_grant;
	uint8_t max_latency;
} __attribute__((packed)) pci_config_device_t;

typedef struct pci_config_bridge_pci_to_pci
{
	uint32_t base_address[2];
	uint8_t primary_bus;
	uint8_t secondary_bus;
	uint8_t subordinate_bus;
	uint8_t secondary_latency_timer;
	uint8_t io_base;
	uint8_t io_limit;
	uint16_t secondary_status;
	uint16_t memory_base;
	uint16_t memory_limit;
	uint16_t prefetchable_memory_base;
	uint16_t prefetchable_memory_limit;
	uint32_t prefetchable_base_upper;
	uint32_t prefetchable_limit_upper;
	uint16_t io_base_upper;
	uint16_t io_limit_upper;
	uint8_t capability_pointer;
	uint8_t reserved[3];
	uint8_t expansion_rom_base_address;
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint16_t bridge_control;
} __attribute__((packed)) pci_config_bridge_pci_to_pci_t;

typedef struct pci_config 
{
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t revision_id;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t class_code;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;

	union
	{
		pci_config_device_t device;
		pci_config_bridge_pci_to_pci_t bridge_pci_to_pci;
	};
	
} __attribute__((packed)) pci_config_t;

typedef struct pci_capability_header
{
	uint8_t id;
	uint8_t next;
} __attribute__((packed)) pci_capability_header_t;

typedef struct pci_capability_msi32
{
	pci_capability_header_t header;
	uint16_t message_control;
	uint32_t message_address;
	uint16_t message_data;
	uint16_t reserved;
	uint32_t mask;
	uint32_t pending;
} __attribute__((packed)) pci_capability_msi32_t;

typedef struct pci_capability_msi64
{
	pci_capability_header_t header;
	uint16_t message_control;
	uint64_t message_address;
	uint16_t message_data;
	uint16_t reserved;
	uint32_t mask;
	uint32_t pending;
} __attribute__((packed)) pci_capability_msi64_t;

typedef struct pci_msix_table_entry
{
	uint64_t message_address;
	uint32_t message_data;
	uint32_t message_control;
} __attribute__((packed)) pci_msix_table_entry_t;

typedef uint64_t pci_msix_pending_entry_t;	/* PBA entry, each bit corresponds to an MSI-X table entry. */

typedef struct pci_capability_msix
{
	pci_capability_header_t header;
	uint16_t message_control;
	uint32_t table_descriptor;							/* Use PCI_MSIX_REG_BAR_ADDR_* macros for accessing the BIR and the offset. */
	uint32_t pending_descriptor;						/* Use PCI_MSIX_REG_BAR_ADDR_* macros for accessing the BIR and the offset. */
} __attribute__((packed)) pci_capability_msix_t;

/* Initialize PCI, detect available access mechanisms. Returns 0 on success, an error code otherwise. */
int pci_init();

/* Enumerate devices on the given bus, add them to the children of <parent>. */
void pci_enumerate_bus(uint8_t bus, device_t* parent);

/* 
 * Returns a device object describing the connected device at the given location (bus, device, function).
 * Returns NULL if no device is found at the given location.
 * Note: This function does not initialize the device nor does it add it to the device tree.
 */
device_pci_t* pci_create_device(uint8_t bus, uint8_t device, uint8_t function);

/* Read an 8 byte value from a devices memory (two 32bit reads)*/
uint64_t pci_read64(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

/* Read a 4 byte value from a devices memory */
uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

/* Read a 2 byte value from a devices memory */
uint16_t pci_read16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

/* Read a byte from a devices memory */
uint8_t pci_read8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

/* Write a 8 byte value to a devices memory. (two 32bit writes) */
void pci_write64(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint64_t value);

/* Write a 4 byte value to a devices memory */
void pci_write32(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value);

/* Write a 2 byte value to a devices memory */
void pci_write16(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value);

/* Write a byte to a devices memory */
void pci_write8(uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value);

/* Read a 4 byte value from a devices memory using access mechanism 1. (CPU IO ports) Note: <offset> must be 4 byte aligned. */
uint32_t pci_read_mechanism1(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/* Write a 4 byte value to a devices memory using access mechanism 1. (CPU IO ports) Note: <offset> must be 4 byte aligned. */
void pci_write_mechanism1(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);