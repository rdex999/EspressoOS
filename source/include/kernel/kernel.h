#pragma once

#include <stdint.h>
#include <multiboot.h>

#ifdef __cplusplus
	extern "C" 
#endif
void kernel_main(multiboot_info_t* mbd);