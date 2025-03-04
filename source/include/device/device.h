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
#include <stdlib.h>

/* Used for down-casting only. */
typedef uint64_t device_type_t;
#define DEVICE_TYPE_NONE 			((device_type_t)0),
#define DEVICE_TYPE_COMPUTER		((device_type_t)1 << 0)
#define DEVICE_TYPE_PCI				((device_type_t)1 << 1)
#define DEVICE_TYPE_PCI_BRIDGE		(((device_type_t)1 << 2) | DEVICE_TYPE_PCI)

class device_t
{
public:
	/* Only initializes device identifiers. This function does not initialize the device nor adds it to the device tree. */
	inline device_t(device_type_t type) 
		: m_type(type), m_parent(NULL), m_children(NULL), m_next(NULL), m_prev(NULL) {}

	/*
	 * This function acts as the destructor of the device.
	 * Destroys all child devices of this device, Removes this device from the device tree.
	 * Uses device::uninitialize(), and free's this device (free(this))
	 * Note: After calling this function, do not use this object anymore.
	 */
	void destroy();

	/* 
	 * Uninitializes the device, free's used resources. 
	 * This function does not remove its children nor does it removes this device from the device tree. 
	 */
	virtual void uninitialize() = 0;

	/* 
	 * Initialize the device and discover all its children (If any). 
	 * This function should be called after adding the device to the device tree.
	 */
	virtual void initialize() = 0;

	/* 
	 * Check if this device matches <dev>, if not check this for all children. (recursive). 
	 * Returns a pointer to the first matching device, NULL on failure.
	 */
	device_t* find(const device_t* dev) const;

	const device_type_t m_type;

protected:
	/* Add a child to the child devices linked list. (Appends to the beginning of the list)*/
	void add_child(device_t* dev);

	/* Remove a child from the child devices linked list. */
	void remove_child(device_t* dev);

	/* 
	 * Check if this device is the same as <dev>, from the most general identifiers to the most specific.
	 * For example if <dev> is a PCI device, then if its class ID doesnt exist, this function would check vendor ID and device ID.
	 */
	virtual bool is_device(const device_t* dev) const = 0;
	
	/* Discover all children of this device. */
	virtual void discover_children() = 0;

	device_t* m_parent;
	device_t* m_children;
	device_t* m_next;
	device_t* m_prev;
};

/* The topmost device in the device tree, it doesnt realy count as a device but it discovers all other devices. */
class device_computer_t : public device_t
{
public:
	device_computer_t() : device_t(DEVICE_TYPE_COMPUTER) {};

	void initialize() 	override {};
	void uninitialize() override {};

	/* 
	 * Made these public so that devices can be added to the root by themselves.
	 * In this class devices are not discovered using discover_children(), the root devices add themselves to the tree.
	 * For example, a PCI device would add itself to the children of the root device.
	 * This is done so devices can add themselves whenever their ready, and not only when the root device is initialized.
	 */
	inline void add_child(device_t* dev) 		{ device_t::add_child(dev); }
	inline void remove_child(device_t* dev) 	{ device_t::remove_child(dev); }

protected:
	bool is_device(const device_t*) const override { return false; };
	void discover_children() override {};
};

extern device_computer_t g_device_root;