SRC=source
BLD=build
CC=gcc
AS=nasm
LD=ld
CFLAGS=
ASFLAGS=-f elf64

all:
	$(MAKE) clean
	$(AS) $(ASFLAGS) -o $(BLD)/boot.obj $(SRC)/x86/boot/boot.asm
	$(LD) -T config/linker.ld -o $(BLD)/kernel.bin $(BLD)/boot.obj
	cp $(BLD)/kernel.bin boot/
	grub-mkstandalone -O x86_64-efi -o $(BLD)/BOOTX64.EFI --directory /usr/lib/grub/x86_64-efi/ boot
	dd if=/dev/zero of=dist/os.img bs=1M count=128
	mkfs.fat -F 32 dist/os.img	
	mmd -i dist/os.img ::/EFI
	mmd -i dist/os.img ::/EFI/BOOT
	mcopy -i dist/os.img build/BOOTX64.EFI ::/EFI/BOOT
	xorriso -as mkisofs -R -f -e dist/os.img -no-emul-boot -o dist/os.iso .

clean:
	rm -rf $(BLD)/*
	rm -f dist/*
	rm -f iso/boot/kernel.bin

run:
	qemu-system-x86_64 -L /usr/share/OVMF/ -pflash /usr/share/OVMF/x64/OVMF_CODE.4m.fd -cdrom dist/os.iso