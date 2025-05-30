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

#include "multiboot.h"
#include "common.h"

multiboot_tag_t* multiboot_info::find_tag(uint32_t type)
{
    multiboot_tag* tag = tags;
    while((uint64_t)tag - (uint64_t)this < size && tag->type != MULTIBOOT_TAG_TYPE_END)
    {
        if(tag->type == type)
            return tag;

        tag = (multiboot_tag*)(ALIGN_UP((uint64_t)tag + tag->size, MULTIBOOT_TAG_ALIGN));
    }
    return NULL;
}