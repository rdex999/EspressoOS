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
#include "acpi/acpi.h"

#define PIC8259_MASTER_IO_COMMAND 	0x20
#define PIC8259_MASTER_IO_DATA 		0x21
#define PIC8259_SLAVE_IO_COMMAND 	0xA1
#define PIC8259_SLAVE_IO_DATA 		0xA1

/* 
 * Initialize all local and IO APIC's on the system. 
 * Returns 0 on success, an error code otherwise.
 */
int apic_init();

/* 
 * Initialize a found IO APIC. Sets IRQ's, things like that
 * Returns 0 on success, an error code otherwise.
 */
int apic_ioapic_init(acpi_madt_record_ioapic_t* ioapic_record);

/* Disable the 8259 PIC. Needed as we are using the APIC. */
void pic8259_disable();

/*  */