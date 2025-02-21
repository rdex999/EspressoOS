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
#include "multiboot.h"
#include "acpi/tables.h"
#include "error.h"

/* Initializes ACPI and finds tables. Returns 0 on success, an error code otherwise. */
int acpi_init(multiboot_info_t* mbd);

/* Find the rsdp/xsdp, and set s_acpi_rsdp to point to it. Returns 0 on success, an error code otherwise. */
int acpi_init_rsdp(multiboot_info_t* mbd);

/* 
 * Check if the table's checksum is valid. 
 * Performed by summing up all bytes in the table and comparing the lowest byte of the sum to 0. 
 */
bool acpi_is_table_valid(void* table, size_t size);