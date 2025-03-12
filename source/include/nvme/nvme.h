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

typedef struct nvme_reg_capabilities				/* CAP */
{
	uint64_t max_queue_entry_count 			: 16;	/* MQES - Maximum amount of queue entries, minus 1. */
	uint64_t contiguous_queue 				: 1;	/* CQR - 1 if is it required that the physical queue memory is contiguous. */
	uint64_t ams_weighted_round_robin		: 1;	/* AMS */
	uint64_t ams_vendor_specific			: 1;	/* AMS */
	uint64_t reserved0 						: 5;	
	uint64_t timeout 						: 8;	/* TO - In 500 milliseconds units. */
	uint64_t stride							: 4;	/* DSTRD - The stride is calculated as: 2**(2 + stride) where "stride" is this field */
	uint64_t subsystem_reset				: 1;	/* NSSRS - 1 if the subsystem reset feature is available, 0 otherwise. */
	uint64_t css_nvm						: 1;	/* "css" stands for Command Set Support. If this field is 1, the NVM command set is supported. */
	uint64_t css_reserved0 					: 1;	/* For any "css" field, if it is 1, the command set it specifies is supported. */
	uint64_t css_reserved1 					: 1;	/* ^ */
	uint64_t css_reserved2 					: 1;	/* | */
	uint64_t css_reserved3 					: 1;	/* | */
	uint64_t css_reserved4 					: 1;	/* | */
	uint64_t css_reserved5 					: 1;	/* | */
	uint64_t css_admin_only 				: 1;	/* If set (1), only the admin command set is available. */
	uint64_t boot_partition_support			: 1;	/* BPS - Set (1) if the controller supports boot partitions. */
	uint64_t reserved1 						: 2;
	uint64_t min_page_size 					: 4;	/* MPSMIN - Minimum supported page size. page size = (2 ^ (12 + MPSMIN)) */
	uint64_t max_page_size					: 4;	/* MPSMAX - Maximum supported page size. page size = (2 ^ (12 + MPSMAN)) */
	uint64_t persistent_mm_region			: 1;	/* PMRS - Set if persistent memory region is supported. */
	uint64_t controller_memory_buffer		: 1;	/* CMBS - Set if the controller supports the Controller Memory Buffer. */
	uint64_t reserved2 						: 6;
} nvme_mmio_capabilities_t;

/* The version number from the mmio configuration space, the VS register. */
typedef struct nvme_reg_version					/* VS */
{
	uint32_t tertiary 	: 8;
	uint32_t minor 		: 8;
	uint32_t major		: 16;
} __attribute__((packed)) nvme_reg_version_t;

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
	void* m_mmio;
};