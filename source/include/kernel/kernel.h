#pragma once

#include <stdint.h>
#include <multiboot2.h>

#ifdef __cplusplus
	extern "C" 
#endif
void kernel_main(multiboot_info_t* mbd);