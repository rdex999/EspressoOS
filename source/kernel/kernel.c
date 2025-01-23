#include "kernel/kernel.h"

#define VIDEO ((uint32_t*)0xA0000)

#ifdef __cplusplus
	extern "C"
#endif
void kernel_main(multiboot_info_t* mbd)
{
	VIDEO[0] = 0x00FF0000;	/* RED */
	VIDEO[1] = 0x0000FF00;	/* GREEN */
	VIDEO[2] = 0x000000FF;	/* BLUE */

	multiboot_tag_mmap_t* tag = (multiboot_tag_mmap_t*)mbd->find_tag(MULTIBOOT_TAG_TYPE_MMAP);
	multiboot_mmap_entry_t* m0 = tag->index(0);
	multiboot_mmap_entry_t* m1 = tag->index(1);
	multiboot_mmap_entry_t* m2 = tag->index(2);


	while(1)
	{
		asm("cli");
		asm("hlt");
	}
}