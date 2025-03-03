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

#include "device/device.h"

// device_computer g_device_root;

void device::destroy()
{
	device* dev = m_children;
	while (dev)
	{
		device* next = dev->m_next;
		
		dev->destroy();
		
		dev = next;
	}

	if(m_parent)
		m_parent->remove_child(this);

	uninitialize();
	free(this);
}

device* device::find(const device* dev) const
{
	if(!dev)
		return NULL;

	if(is_device(dev))
		return (device*)this;
	
	device* child = m_children;
	while (child)
	{
		if(device* found = child->find(dev))
			return found;

		child = child->m_next;
	}

	return NULL;
}

void device::add_child(device* dev)
{
	if(!dev)
		return;

	dev->m_next = m_children;
	m_children->m_prev = dev;
	m_children = dev;
	dev->m_parent = this;
}

void device::remove_child(device* dev)
{
	if(!dev)
		return;

	if (dev->m_prev)
		dev->m_prev->m_next = dev->m_next;

	if (dev->m_next)
		dev->m_next->m_prev = dev->m_prev;

	if (m_children == dev)
		m_children = dev->m_next;

	dev->m_parent = NULL;
	dev->m_prev = NULL;
	dev->m_next = NULL;
}