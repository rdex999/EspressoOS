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

device_computer_t g_device_root;

void device_t::destroy()
{
	device_t* device = m_children;
	while (device)
	{
		device_t* next = device->m_next;
		
		device->destroy();
		
		device = next;
	}

	if(m_parent)
		m_parent->remove_child(this);

	uninitialize();
	free(this);
}

device_t* device_t::find(const device_t* device) const
{
	if(!device)
		return NULL;

	if(is_device(device))
		return (device_t*)this;
	
	device_t* child = m_children;
	while (child)
	{
		if(device_t* found = child->find(device))
			return found;

		child = child->m_next;
	}

	return NULL;
}

void device_t::add_child(device_t* device)
{
	if(!device)
		return;

	if(m_children)
		m_children->m_prev = device;

	device->m_next = m_children;
	m_children = device;
	device->m_parent = this;
}

void device_t::remove_child(device_t* device)
{
	if(!device)
		return;

	if (device->m_prev)
		device->m_prev->m_next = device->m_next;

	if (device->m_next)
		device->m_next->m_prev = device->m_prev;

	if (m_children == device)
		m_children = device->m_next;

	device->m_parent = NULL;
	device->m_prev = NULL;
	device->m_next = NULL;
}