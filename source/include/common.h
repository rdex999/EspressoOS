#pragma once

// Works only when align is a power of 2.
// Basicaly, when subtracting 1 from align, all bits from right, until the original bit of align, are set to 1. So, we get a mask So, we get a mask.
// So add the mask to the address, that way the address will "have" the mask.
// Then, remove remove the mask from the address so it will be aligned upwards.
#define ALIGN(address, align) (address + (align - 1)) & ~(align - 1)