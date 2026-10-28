.PHONY: default build

default:
	$(error Do not run this file directly!)

build:
	mkdir -p iso_tmp_data
	mkdir -p tmp_rootfs
	cp -r tools/rootfs/* tmp_rootfs/
ifeq ($(BUILD_DRIVERS), 1)
	cp -r Drivers/out/* tmp_rootfs/sys/drv/
endif
ifeq ($(BUILD_USERSPACE), 1)
	cp -r Userspace/out/* tmp_rootfs/
endif
	chmod -R 755 tmp_rootfs/sys/
	chmod -R 755 tmp_rootfs/usr/
	chmod 755 tmp_rootfs/home/
	chmod -R 750 tmp_rootfs/home/root/
	chmod -R 777 tmp_rootfs/tmp/
	tar czf rootfs.tar.gz -C tmp_rootfs/ --owner=root:0 --group=root:0 ./ --format=ustar
	cp Kernel/build/fennix-*.elf iso_tmp_data/fennix.elf
	cp rootfs.tar.gz iso_tmp_data/
ifeq ($(BOOTLOADER), limine)
	cp  tools/limine.conf \
		tools/limine/limine-bios.sys \
		tools/limine/limine-bios-cd.bin \
		tools/limine/limine-uefi-cd.bin \
		iso_tmp_data/
	mkdir -p iso_tmp_data/EFI/BOOT
	cp  tools/limine/BOOTAA64.EFI \
		tools/limine/BOOTIA32.EFI \
		tools/limine/BOOTLOONGARCH64.EFI \
		tools/limine/BOOTRISCV64.EFI \
		tools/limine/BOOTX64.EFI \
		iso_tmp_data/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_tmp_data -o Fennix.iso
endif
ifeq ($(BOOTLOADER), grub)
# TODO: Add custom language support for GRUB or detect the system language using "echo $LANG | cut -d . -f 1" and set "lang" variable inside grub.cfg
	mkdir -p iso_tmp_data/boot
	mkdir -p iso_tmp_data/boot/grub
	cp tools/grub.cfg iso_tmp_data/boot/grub/
	grub-mkrescue -d /usr/lib/grub/x86_64-efi -o Fennix.iso iso_tmp_data
endif
ifneq ($(filter aarch64 arm,$(OSARCH)),)
	$(__CONF_OBJCOPY) Kernel/build/fennix.elf -O binary Fennix.img
#	cp Bootloader/boot.bin Fennix.img
endif
