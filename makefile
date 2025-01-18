SRC=source
BLD=build
CC=gcc
AS=nasm
LD=ld
CFLAGS=
ASFLAGS=-f elf64
QEMU_FLAGS=-m 8192M -vga vmware -L /usr/share/OVMF/ -pflash /usr/share/OVMF/x64/OVMF_CODE.4m.fd
DISK_IMG=dist/EspressoOS.img
DISK_IMG_SIZE=$$((1 * 1024**3))

# This rule requires root privileges for mounting and formating disk image partitions
all:
	$(AS) $(ASFLAGS) -o $(BLD)/boot.obj $(SRC)/x86/boot/boot.asm
	$(LD) -T config/linker.ld -o $(BLD)/kernel.bin $(BLD)/boot.obj

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

clean:
	rm -rf $(BLD)/*
	rm -f dist/*

run:
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=$(DISK_IMG)