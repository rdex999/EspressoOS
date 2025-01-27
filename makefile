# 
# This file is part of the EspressoOS project (https://github.com/rdex999/EspressoOS.git).
# Copyright (c) 2025 David Weizman.
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
# 
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

SRC=source
BLD=build
CC=g++
AS=nasm
LD=ld
export CFLAGS+=-m64 -c -ffreestanding -Wall -Wextra \
	-fno-stack-protector -fno-exceptions -fno-rtti 	\
	-I $(SRC)/include -I libk/include
export ASFLAGS+=-f elf64 -I $(SRC)
QEMU_FLAGS=-m 8G -vga vmware -L /usr/share/OVMF/ -pflash /usr/share/OVMF/x64/OVMF_CODE.4m.fd
ISO=dist/EspressoOS.iso
DISK_IMG=dist/EspressoOS.img
DISK_IMG_SIZE=$$((1 * 1024**3))
DEBUG_BREAKPOINT=kernel_main

.DEFAULT_GOAL=iso

.PHONY: all image iso clean rundisk runiso debugimage debugiso

all:
	mkdir -p $(BLD)/libk

# This rule requires root privileges for mounting and formating disk image partitions
image: all $(DISK_IMG)
$(DISK_IMG): $(BLD)/kernel.bin
	@# Create the bootloader image, it uses the grub config in config/grub/boot_grub.cfg
	grub-mkstandalone -O x86_64-efi -o build/BOOTX64.EFI 	\
		"boot/grub/grub.cfg=config/grub/boot_grub.cfg" 		\
		--directory /usr/lib/grub/x86_64-efi/

	@# Create the disk image
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=1024

	@# Create partitions on the dist image using GPT
	@# See parted manual for more detailes
	@# "-s" for no prompting, "-a minimal" means minimal alignment when creating partitions.

	@# Create GPT partition table
	parted $(DISK_IMG) -s -a minimal mklabel gpt                             

	@# Create a 260MiB EFI system partition named "ESP". 260MiB is the recomended size for an ESP.
	parted $(DISK_IMG) -s -a minimal mkpart ESP fat32 1MiB 261MiB

	@# Create the OS partition, starting from the 261MiB until the end of the image.
	parted $(DISK_IMG) -s -a minimal mkpart UEFIOS64 ext2 261MiB 100%

	@# Toggle on the "boot" flag for the ESP.
	parted $(DISK_IMG) -s -a minimal toggle 1 boot

	@# Format the ESP, put the EFI bootloader in there
	losetup --offset $$(( 1 * 1024**2 )) --sizelimit $$((261 * 1024**2)) /dev/loop0 $(DISK_IMG)
	mkfs.fat -F 32 -n "EFI System" /dev/loop0
	mount /dev/loop0 /mnt
	mkdir -p /mnt/EFI/BOOT
	cp $(BLD)/BOOTX64.EFI /mnt/EFI/BOOT
	umount /mnt
	losetup -d /dev/loop0
	sync

	@# Format the os partition, put things in it
	losetup --offset $$((1024**2 * 261)) --sizelimit $(DISK_IMG_SIZE) /dev/loop0 $(DISK_IMG)
	mkfs.ext2 -L "EspressoOS" /dev/loop0
	mount /dev/loop0 /mnt
	mkdir -p /mnt/boot/grub
	cp config/grub/disk_grub.cfg /mnt/boot/grub/grub.cfg
	cp $(BLD)/kernel.bin /mnt/boot
	umount /mnt
	losetup -d /dev/loop0
	sync

iso: all $(ISO)
$(ISO): $(BLD)/kernel.bin
	@# The "iso_disk" directory is the skeleton for the ISO image, so everything under it will be in the ISO image.

	@# The config file should be located in the disk image at /boot/grub/grub.cfg,
	@# and the kernel should be located at /boot/kernel.bin, so put it there.
	mkdir -p iso_disk/boot/grub
	cp config/grub/iso_grub.cfg iso_disk/boot/grub/grub.cfg
	cp $(BLD)/kernel.bin iso_disk/boot

	@# Create the ISO image.
	grub-mkrescue -o $@ iso_disk 

$(BLD)/kernel.bin: $(BLD)/kernel.obj $(BLD)/boot.obj $(BLD)/multiboot.obj $(BLD)/libk/libk.lib $(BLD)/pmm.obj
	$(LD) -T config/linker.ld -o $@ $^

$(BLD)/%.obj: $(SRC)/%.c $(SRC)/include/%.h
	$(CC) $(CFLAGS) -o $@ $<

$(BLD)/boot.obj: $(SRC)/x86/boot/boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

$(BLD)/kernel.obj: $(SRC)/kernel/kernel.c $(SRC)/include/kernel/kernel.h
	$(CC) $(CFLAGS) -o $@ $<

$(BLD)/pmm.obj: $(SRC)/mm/pmm/pmm.c $(SRC)/include/mm/pmm/pmm.h
	$(CC) $(CFLAGS) -o $@ $<

$(BLD)/libk/libk.lib: $(BLD)/libk/string.obj
	ld -r -o $@ $^

$(BLD)/libk/string.obj: libk/source/string.c libk/include/string.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BLD)/* dist/* iso_disk

runimage: image
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$(DISK_IMG)

runiso: iso
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom $(ISO)

debugimage:
	$(MAKE) clean
	$(MAKE) image CFLAGS+=-g ASFLAGS+=-g
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$(DISK_IMG) -S -s &
	gdb $(BLD)/kernel.bin 								\
        -ex "target remote localhost:1234" 				\
        -ex "set disassembly-flavor intel" 				\
        -ex "break $(DEBUG_BREAKPOINT)" -ex "continue" 	\
        -ex "lay src"

debugiso:
	$(MAKE) clean
	$(MAKE) iso CFLAGS+=-g ASFLAGS+=-g
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom $(ISO) -S -s &
	gdb $(BLD)/kernel.bin 								\
		-ex "target remote localhost:1234" 				\
		-ex "set disassembly-flavor intel" 				\
		-ex "break $(DEBUG_BREAKPOINT)" -ex "continue" 	\
		-ex "lay src"