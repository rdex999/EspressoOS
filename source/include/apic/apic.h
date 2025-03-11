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

#define PIC8259_MASTER_IO_COMMAND 			0x20
#define PIC8259_MASTER_IO_DATA 				0x21
#define PIC8259_SLAVE_IO_COMMAND 			0xA1
#define PIC8259_SLAVE_IO_DATA 				0xA1

/* 
 * When reading/writing a register into the IO APIC's configuration space, 
 * write the register index into IOREGSEL and then read/write it from/to IOREGWIN.
 */
#define APIC_IOAPIC_IOREGSEL				0
#define APIC_IOAPIC_IOREGWIN				0x10

#define APIC_IOAPIC_REG_IOAPICID			0						/* IOAPIC id */
#define APIC_IOAPIC_REG_IOAPICVER 			1						/* IOAPIC version */
#define APIC_IOAPIC_REG_IOAPICARB			2						/* IOAPIC arbitration id */
#define APIC_IOAPIC_REG_IRQ_0				0x10
#define APIC_IOAPIC_REG_IOREDTBL(irq)		(APIC_IOAPIC_REG_IRQ_0 + 2 * (irq)) /* IOAPIC redirection entry register for a IRQ <irq> */

#define APIC_IOAPIC_IOAPICVER_MAX_REDTBL(reg)	(((reg) >> 16) & 0xFF)			/* The highest redtbl index (register) */
#define APIC_IOAPIC_REDTBL_TO_IRQ(redtbl_idx)	(((redtbl_idx) - 0x10) / 2)		/* redtbl index (register) to IRQ number */

#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_FIXED			0
#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_LOW_PRIORITY	1
#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_SYSTEM_MNG_INT 2
#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_NMI			4
#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_INIT			5
#define APIC_IOAPIC_REDTBL_DELIVERY_MODE_EXTERNAL		7

#define APIC_IOAPIC_REDTBL_DESTINATION_MODE_PHYSICAL	0
#define APIC_IOAPIC_REDTBL_DESTINATION_MODE_LOGICAL		1

#define APIC_IOAPIC_REDTBL_PIN_POLARITY_ACTIVE_HIGH 	0
#define APIC_IOAPIC_REDTBL_PIN_POLARITY_ACTIVE_LOW 		1

#define APIC_IOAPIC_REDTBL_TRIGGER_MODE_EDGE			0
#define APIC_IOAPIC_REDTBL_TRIGGER_MODE_LEVEL			1

#define APIC_LAPIC_REG_SPURIOUS_INTERRUPT_VECTOR	0xF0

typedef struct apic_ioapic_descriptor
{
	uint8_t* mmio;
	uint8_t first_irq;
	struct apic_ioapic_descriptor* next;
} apic_ioapic_descriptor_t;

typedef struct apic_ioapic_redtbl_entry
{
	uint64_t interrupt 			: 8;
	uint64_t delivery_mode 		: 3;
	uint64_t destination_mode	: 1;
	uint64_t delivery_status	: 1;
	uint64_t pin_polarity		: 1;
	uint64_t remote_irr			: 1;
	uint64_t trigger_mode 		: 1;
	uint64_t mask 				: 1;
	uint64_t reserved 			: 39;
	uint64_t destination		: 8;
} __attribute__((packed)) apic_ioapic_redtbl_entry_t;

/* 
 * Initialize all local and IO APIC's on the system. 
 * Returns 0 on success, an error code otherwise.
 */
int apic_init();

/* Disable the 8259 PIC. Needed as we are using the APIC. */
void pic8259_disable();

/* 
 * Map IRQ <irq> to interrupt vector <interrupt>.
 * Returns 0 on success, an error code otherwise.
 */
int apic_map_irq(uint8_t irq, uint8_t interrupt);

/* 
 * Initialize a found IO APIC. Sets IRQ's, things like that
 * Returns 0 on success, an error code otherwise.
 */
int apic_ioapic_init(acpi_madt_record_ioapic_t* ioapic_record);

/* 
 * Map <irq> of <ioapic> to interrupt vector <interrupt>. 
 * Returns 0 on success, an error code otherwise.
 */
int apic_ioapic_map_irq(const apic_ioapic_descriptor_t* ioapic, uint8_t irq, uint8_t interrupt);

/* Check if IRQ <irq> is in the IRQ range of IO APIC <ioapic>. Returns true if in range, false if not. */
bool apic_ioapic_irq_in_range(const apic_ioapic_descriptor_t* ioapic, uint8_t irq);

/* Read a 32 bit register from an IO APIC's configuration space. */
uint32_t apic_ioapic_read32(const apic_ioapic_descriptor_t* ioapic, uint8_t reg);

/* Read a 64 bit register from an IO APIC's configuration space. */
uint64_t apic_ioapic_read64(const apic_ioapic_descriptor_t* ioapic, uint8_t reg);

/* Write a 32 bit value into a register in an IO APIC's configuration space. */
void apic_ioapic_write32(const apic_ioapic_descriptor_t* ioapic, uint8_t reg, uint32_t value);

/* Write a 64 bit value into a register in an IO APIC's configuration space. */
void apic_ioapic_write64(const apic_ioapic_descriptor_t* ioapic, uint8_t reg, uint64_t value);

/* 
 * Initialize the local APIC of the current CPU. 
 * Return 0 on success, an error code otherwise.
 */
int apic_lapic_init();

/* 
 * Get the virtual address representing the physical address of the configuration space for the local APIC of the current CPU. 
 * Returns a valid virtual address on success, NULL on failure. 
 */
uint64_t apic_lapic_get_mmio();

/* Read a register in the local APIC configuration space. */
uint32_t apic_lapic_read_reg(uint16_t reg);

/* Write to a register in the local APIC configuration space. Returns 0 on failure. */
void apic_lapic_write_reg(uint16_t reg, uint32_t value);