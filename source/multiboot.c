#include "multiboot.h"
#include "common.h"

multiboot_tag_t* multiboot_info::find_tag(uint32_t type)
{
    multiboot_tag* tag = this->tags;
    while(tag - this->tags < size)
    {
        if(tag->type == type)
            return tag;

        tag = (multiboot_tag*)(ALIGN((uint64_t)tag + tag->size, MULTIBOOT_TAG_ALIGN));
    }
    return NULL;
}