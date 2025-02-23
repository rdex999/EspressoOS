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

/* Read a 4 byte value from a devices memory using memory mapped access. */
uint32_t pci_read_enhanched(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/* Write a 4 byte value to a devices memory using access mechanism 1. (CPU IO ports)*/
uint32_t pci_write_mechanism1(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/* Write a 4 byte value to a devices memory using memory mapped access. */
void pci_write_enhanched(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);