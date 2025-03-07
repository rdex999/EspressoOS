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

/* 
 * Initialize all local and IO APIC's on the system. 
 * Returns 0 on success, an error code otherwise.
 */
int apic_init();

/* 
 * Initialize a found IO APIC. Sets IRQ's, things like that
 * Note: <ioapic_phys_base> is the physical address of the IO APIC configuration space.
 * Returns 0 on success, an error code otherwise.
 */
int apic_ioapic_init(uint64_t ioapic_phys_base);

/* 
 * Disable the 8259 PIC. Needed as we are using the APIC.
 * Returns 0 on success, an error code otherwise. 
 */
int apic_disable_pic8259();