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

#include "apic/apic.h"

#include "error.h"
#include "cpu.h"

int apic_init()
{
	pic8259_disable();

	return SUCCESS;
}

void pic8259_disable()
{
	/* Mask all interrupts on the master and slave PIC's */
    outb(PIC8259_MASTER_IO_DATA, 0xff);
    outb(PIC8259_SLAVE_IO_DATA, 0xff);
}