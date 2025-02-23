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
} __attribute__((packed)) pci_config_t;

typedef struct pci_config_device
{
	pci_config_t config;
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

/* Initialize PCI, detect available access mechanisms. Returns 0 on success, an error code otherwise. */
int pci_init();

/* Read a 4 byte value from a devices memory */
uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Read a 2 byte value from a devices memory */
uint32_t pci_read16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Read a byte from a devices memory */
uint32_t pci_read8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Write a 4 byte value to a devices memory */
void pci_write32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/* Write a 2 byte value to a devices memory */
void pci_write16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t value);

/* Write a byte to a devices memory */
void pci_write8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t value);

/* Read a 4 byte value from a devices memory using access mechanism 1. (CPU IO ports)*/
uint32_t pci_read_mechanism1(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Read a 4 byte value from a devices memory using memory mapped configuration. */
uint32_t pci_read_mmconfig(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Write a 4 byte value to a devices memory using access mechanism 1. (CPU IO ports)*/
uint32_t pci_write_mechanism1(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/* Write a 4 byte value to a devices memory using memory mapped configuration. */
void pci_write_mmconfig(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);