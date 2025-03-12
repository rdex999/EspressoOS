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

#include "device/device.h"
#include "storage/storage.h"
#include "pci/pci.h"

/* 
 * For detailes about MMIO registers, and other stuff see the NVMe 1.4 specification at: 
 * https://nvmexpress.org/wp-content/uploads/NVM-Express-1_4-2019.06.10-Ratified.pdf 
 * Starting from page 42, there is an explanation about all MMIO registers.
 */

typedef struct nvme_reg_capabilities				/* CAP */
{
	uint64_t max_queue_entry_count 			: 16;	/* MQES - Maximum amount of queue entries, minus 1. */
	uint64_t contiguous_queue 				: 1;	/* CQR - 1 if is it required that the physical queue memory is contiguous. */
	uint64_t ams_weighted_round_robin		: 1;	/* AMS */
	uint64_t ams_vendor_specific			: 1;	/* AMS */
	uint64_t reserved0 						: 5;	
	uint64_t timeout 						: 8;	/* TO - In 500 milliseconds units. */
	uint64_t stride							: 4;	/* DSTRD - The stride is calculated as: 2**(2 + DSTRD) */
	uint64_t subsystem_reset				: 1;	/* NSSRS - 1 if the subsystem reset feature is available, 0 otherwise. */
	uint64_t css_nvm						: 1;	/* "css" stands for Command Set Support. If this field is 1, the NVM command set is supported. */
	uint64_t css_reserved0 					: 1;	/* For any "css" field, if it is 1, the command set it specifies is supported. */
	uint64_t css_reserved1 					: 1;	/* ^ */
	uint64_t css_reserved2 					: 1;	/* | */
	uint64_t css_reserved3 					: 1;	/* | */
	uint64_t css_reserved4 					: 1;	/* | */
	uint64_t css_reserved5 					: 1;	/* | */
	uint64_t css_admin_only 				: 1;	/* If set (1), only the admin command set is available. */
	uint64_t boot_part_support				: 1;	/* BPS - Set (1) if the controller supports boot partitions. */
	uint64_t reserved1 						: 2;
	uint64_t min_page_size 					: 4;	/* MPSMIN - Minimum supported page size. page size = (2 ^ (12 + MPSMIN)) */
	uint64_t max_page_size					: 4;	/* MPSMAX - Maximum supported page size. page size = (2 ^ (12 + MPSMAN)) */
	uint64_t persistent_mm_region			: 1;	/* PMRS - Set if persistent memory region is supported. */
	uint64_t controller_memory_buffer		: 1;	/* CMBS - Set if the controller supports the Controller Memory Buffer. */
	uint64_t reserved2 						: 6;
} __attribute__((packed)) nvme_reg_capabilities_t;

/* The version number from the mmio configuration space, the VS register. */
typedef struct nvme_reg_version					/* VS */
{
	uint32_t tertiary 	: 8;
	uint32_t minor 		: 8;
	uint32_t major		: 16;
} __attribute__((packed)) nvme_reg_version_t;

typedef struct nvme_registers
{
	nvme_reg_capabilities_t capabilities;							/* CAP */
	nvme_reg_version_t 		version;								/* VS */
	uint32_t 				int_mast_set;							/* INTMS */
	uint32_t 				int_mast_clear;							/* INTMC */
	uint32_t 				configuration;							/* CC - Controller Configuration */
	uint32_t				reserved0;
	uint32_t 				status;									/* CSTS - Controller Status */
	uint32_t				subsystem_reset;						/* NSSR - NVM Subsystem Reset */
	uint32_t 				queue_attr;								/* AQA - Admin Queue Attributes */
	uint64_t 				admin_sbms_queue_addr;					/* ASQ - Admin Submission Queue Base Address */
	uint64_t 				admin_cmpl_queue_addr;					/* ACQ - Admin Completion Queue Base Address */
	uint32_t				memory_buffer_addr;						/* CMBLOC - Controller Memory Buffer Location */
	uint32_t				memory_buffer_size;						/* CMBLSZ - Controller Memory Buffer Size */
	uint32_t				boot_part_info;							/* BPINFO - Boot Partition Information */
	uint32_t				boot_part_read_select;					/* BPRSEL - Boot Partition Read Select */
	uint64_t				boot_part_mm_buffer_addr;				/* BPMBL - Boot Partition Memory Buffer Location */
	uint64_t				mm_buffer_mm_space_control;				/* CMBMSC - Controller Memory Buffer Memory Space Control */
	uint32_t				mm_buffer_status;						/* CMBSTS - Controller Memory Buffer Status */
	uint8_t 				reserved1[0xDFF - 0x5C + 1];
	uint32_t				prstnt_mm_capabilities; 				/* PMRCAP - Persistent Memory Region Capabilities */
	uint32_t				prstnt_mm_control; 						/* PMRCTL - Persistent Memory Region Control */
	uint32_t				prstnt_mm_status; 						/* PMRSTS - Persistent Memory Region Status */
	uint32_t				prstnt_mm_elasticity_buffer_size; 		/* PMREBS - Persistent Memory Region Elasticity Buffer Size */
	uint32_t				prstnt_mm_sstnd_write_throughput; 		/* PMRSWTP - Persistent Memory Region Sustained Write Throughput */
	uint64_t				prstnt_mm_ctrl_mm_space_control; 		/* PMRMSC - Persistent Memory Region Controller Memory Space Control */
	uint8_t 				reserved2[0xFFF - 0xE1C + 1];			/* Command set specific */
} __attribute__((packed)) nvme_registers_t;

class device_storage_pci_nvme_t : public device_storage_t, public device_pci_t
{
public:	
	device_storage_pci_nvme_t(uint8_t bus, uint8_t device, uint8_t function) : 
		device_t(DEVICE_TYPE_STORAGE | DEVICE_TYPE_PCI | DEVICE_TYPE_NVME, this),
		device_pci_t(DEVICE_TYPE_STORAGE | DEVICE_TYPE_PCI | DEVICE_TYPE_NVME, bus, device, function) {}

	int initialize() override;
	int uninitialize() override;

	void discover_children() override {};
	
protected:
	inline bool is_device(const device_t* device) const override { return device_pci_t::is_device(device); };

	int read_sectors(uint64_t lba, size_t count, void* buffer) const override;
	int write_sectors(uint64_t lba, size_t count, const void* buffer) const override;

private:
	/* The memory mapped registers used by the NVME controller. */
	nvme_registers_t* m_mmio;
};