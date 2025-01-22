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

	while(1){
		asm("cli");
		asm("hlt");
	}
}