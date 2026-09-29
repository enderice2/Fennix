
include config.mk

.PHONY: default tools clean ci-build build run

# First rule
default:
	$(error Please specify a target)

# For tap0
# -netdev tap,id=usernet0,ifname=tap0,script=no,downscript=no
QEMU = $(__CONF_QEMU_PATH)
QEMUFLAGS := -display gtk

ifeq ($(OSARCH), amd64)
QEMUFLAGS += -device vmware-svga -M q35 \
			 -cpu max \
			 \
			 -device piix3-usb-uhci,id=uhci \
			 -device usb-mouse,bus=uhci.0,pcap=mouse.pcap \
			 -device usb-kbd,bus=uhci.0,pcap=kbd.pcap \
			 -device usb-tablet,bus=uhci.0 \
			 -device usb-wacom-tablet,bus=uhci.0 \
			 -device usb-ccid,bus=uhci.0 \
			 \
			 -device pci-ohci,id=ohci \
			 -device usb-mouse,bus=ohci.0 \
			 -device usb-kbd,bus=ohci.0 \
			 -device usb-tablet,bus=ohci.0 \
			 -device usb-wacom-tablet,bus=ohci.0 \
			 -device usb-ccid,bus=ohci.0 \
			 \
			 -device usb-ehci,id=ehci \
			 -device usb-mouse,bus=ehci.0 \
			 -device usb-kbd,bus=ehci.0 \
			 \
			 -device qemu-xhci,id=xhci \
			 -device usb-mouse,bus=xhci.0 \
			 -device usb-kbd,bus=xhci.0 \
			 -device usb-tablet,bus=xhci.0 \
			 -device usb-wacom-tablet,bus=xhci.0 \
			 -device usb-ccid,bus=xhci.0 \
			 \
			 -net user \
			 -netdev user,id=usernet0 \
			 -device e1000,netdev=usernet0,mac=00:69:96:00:42:00 \
			 -object filter-dump,id=usernet0,netdev=usernet0,file=network.dmp,maxlen=1024 \
			 \
			 -trace usb_uhci_* \
			 -trace usb_ohci_* \
			 -trace usb_ehci_* \
			 -trace usb_xhci_* \
			 -trace usb_hub_* \
			 -trace usb_port_* \
			 -trace ps2_* \
			 -serial file:serial.log \
			 -serial file:COM2.dmp \
			 -serial file:COM3.dmp \
			 -serial stdio \
			 -parallel file:LPT1.dmp \
			 -parallel file:LPT2.dmp \
			 -parallel file:LPT3.dmp \
			 \
			 -device ahci,id=ahci \
			 -drive id=bootdsk,file=$(OSNAME).iso,format=raw,if=none \
			 -device ide-hd,drive=bootdsk,bus=ahci.0 \
			 -drive id=disk,file=qemu-disk.qcow2,if=none \
			 -device ide-hd,drive=disk,bus=ahci.1 \
			 -audiodev pa,id=pa1,server=/run/user/1000/pulse/native \
			 -machine pcspk-audiodev=pa1 \
			 -device AC97,audiodev=pa1 \
			 -device intel-hda \
			 -device ich9-intel-hda \
			 -acpitable file=tools/acpi/SSDT1.dat
else ifeq ($(OSARCH), i386)
QEMUFLAGS += -M q35 \
			 -cpu max \
			 -usb \
			 -device usb-tablet,bus=usb-bus.0,port=1 \
			 -device usb-hub,bus=usb-bus.0,port=2 \
			 -device qemu-xhci,id=xhci \
			 -device usb-mouse,bus=xhci.0 \
			 -device usb-kbd,bus=xhci.0 \
			 -device usb-mouse,pcap=mouse.pcap \
			 -device usb-kbd,pcap=kbd.pcap \
			 -net user \
			 -netdev user,id=usernet0 \
			 -device e1000,netdev=usernet0,mac=00:69:96:00:42:00 \
			 -object filter-dump,id=usernet0,netdev=usernet0,file=network.dmp,maxlen=1024 \
			 -serial file:serial.log \
			 -serial file:COM2.dmp \
			 -serial file:COM3.dmp \
			 -serial stdio \
			 -parallel file:LPT1.dmp \
			 -parallel file:LPT2.dmp \
			 -parallel file:LPT3.dmp \
			 -hda $(OSNAME).iso \
			 -audiodev pa,id=pa1,server=/run/user/1000/pulse/native \
			 -machine pcspk-audiodev=pa1 \
			 -device AC97,audiodev=pa1 \
			 -device intel-hda \
			 -device ich9-intel-hda \
			 -acpitable file=tools/acpi/SSDT1.dat
else ifeq ($(OSARCH), arm)
QEMUFLAGS += -M raspi2b \
			 -cpu cortex-a15 \
			 -serial file:serial.log \
			 -serial file:COM2.dmp \
			 -serial file:COM3.dmp \
			 -serial stdio \
			 -kernel $(OSNAME).img
else ifeq ($(OSARCH), aarch64)
QEMUFLAGS += -M raspi4b \
			 -cpu cortex-a72 \
			 -serial file:serial.log \
			 -serial file:COM2.dmp \
			 -serial file:COM3.dmp \
			 -serial stdio \
			 -kernel $(OSNAME).img
endif

ifeq ($(QUIET_BUILD), 1)
MAKE_QUIET_FLAG := --quiet
endif

doxygen:
	mkdir -p doxygen-doc
	doxygen Doxyfile

qemu_vdisk:
ifneq (,$(wildcard ./qemu-disk.qcow2))
	$(info qemu-disk.qcow2 Already exists)
else
	qemu-img create -f qcow2 qemu-disk.qcow2 1G
endif

# Install necessary packages, build cross-compiler etc...
tools:
	$(MAKE) --quiet -C tools all

prepare:
	$(MAKE) --quiet -C Kernel prepare
	$(MAKE) --quiet -C Bootloader prepare
	$(MAKE) --quiet -C Drivers prepare
	$(MAKE) --quiet -C Userspace prepare

MKDIR_ROOTFS = mkdir -p rootfs
mkdir_rootfs:
	$(MKDIR_ROOTFS)/home/root/apps/
	$(MKDIR_ROOTFS)/home/root/cfg/
	$(MKDIR_ROOTFS)/home/root/cache/
	$(MKDIR_ROOTFS)/home/root/trash/
	$(MKDIR_ROOTFS)/home/root/desktop/
	$(MKDIR_ROOTFS)/home/root/documents/
	$(MKDIR_ROOTFS)/home/root/downloads/
	$(MKDIR_ROOTFS)/home/root/music/
	$(MKDIR_ROOTFS)/home/root/pictures/
	$(MKDIR_ROOTFS)/home/root/videos/
	$(MKDIR_ROOTFS)/home/root/templates/

	$(MKDIR_ROOTFS)/sys/bin/
	$(MKDIR_ROOTFS)/sys/lib/
	$(MKDIR_ROOTFS)/sys/drv/
	$(MKDIR_ROOTFS)/sys/svc/
	$(MKDIR_ROOTFS)/sys/cfg/
	$(MKDIR_ROOTFS)/sys/inc/
	$(MKDIR_ROOTFS)/sys/res/
	$(MKDIR_ROOTFS)/sys/log/panic/

	$(MKDIR_ROOTFS)/usr/bin/
	$(MKDIR_ROOTFS)/usr/lib/
	$(MKDIR_ROOTFS)/usr/share/
	$(MKDIR_ROOTFS)/usr/include/

	$(MKDIR_ROOTFS)/mnt/
	$(MKDIR_ROOTFS)/tmp/

setup:
	$(MAKE) prepare
	$(MAKE) tools
	$(MAKE) mkdir_rootfs

setup-no-qemu:
	$(MAKE) --quiet -C tools ci

build:
	$(MAKE) $(MAKE_QUIET_FLAG) mkdir_rootfs
	$(MAKE) $(MAKE_QUIET_FLAG) build_kernel
	$(MAKE) $(MAKE_QUIET_FLAG) build_bootloader
	$(MAKE) $(MAKE_QUIET_FLAG) build_userspace
	$(MAKE) $(MAKE_QUIET_FLAG) build_drivers
	$(MAKE) $(MAKE_QUIET_FLAG) build_image

dump:
	$(MAKE) --quiet -C Kernel dump

rebuild: clean build

__ci-build-set-release:
	sed -i 's/.*DEBUG = .*/DEBUG = 0/' ./config.mk && cat config.mk | grep DEBUG

__ci-build-set-debug:
	sed -i 's/.*DEBUG = .*/DEBUG = 1/' ./config.mk && cat config.mk | grep DEBUG

ci-setup:
	$(MAKE) --quiet -C tools ci

__ci-amd64-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-amd64-debug.iso
	$(MAKE) clean

__ci-amd64-release:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-amd64-release.iso
	$(MAKE) clean

__ci-i386-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = i386/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-i386-debug.iso
	$(MAKE) clean

__ci-i386-release:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = i386/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-i386-release.iso
	$(MAKE) clean

__ci-arm-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = arm/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-arm-debug.iso
	$(MAKE) clean

__ci-arm-release:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = arm/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-arm-release.iso
	$(MAKE) clean

__ci-aarch64-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = aarch64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-aarch64-debug.iso
	$(MAKE) clean

__ci-aarch64-release:
	$(MAKE) -C tools do_limine
	$(MAKE) __ci-build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = aarch64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-aarch64-release.iso
	$(MAKE) clean

__ci-restore-config:
	$(MAKE) __ci-build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH

__ci-prepare-archive:
	mkdir -p artifacts
	$(MAKE) changelog
	cp -f CHANGELOG.md artifacts/
	mv -f Fennix-*.iso artifacts/

ci-build:
# Prepare
	$(MAKE) prepare
	$(MAKE) mkdir_rootfs
	$(MAKE) clean

	$(MAKE) __ci-amd64-debug
	$(MAKE) __ci-amd64-release
	$(MAKE) __ci-i386-debug
	$(MAKE) __ci-i386-release
	$(MAKE) __ci-arm-debug
	$(MAKE) __ci-arm-release
	$(MAKE) __ci-aarch64-debug
	$(MAKE) __ci-aarch64-release
	$(MAKE) __ci-restore-config
	$(MAKE) __ci-prepare-archive

changelog:
	git cliff > CHANGELOG.md

build_kernel:
ifeq ($(BUILD_KERNEL), 1)
	$(MAKE) -j$(shell nproc) $(MAKE_QUIET_FLAG) -C Kernel build
endif

build_bootloader:
ifeq ($(BUILD_BOOTLOADER), 1)
	$(MAKE) $(MAKE_QUIET_FLAG) -C Bootloader build
endif

build_drivers:
ifeq ($(BUILD_DRIVERS), 1)
	$(MAKE) $(MAKE_QUIET_FLAG) -C Drivers build
endif

build_userspace:
ifeq ($(BUILD_USERSPACE), 1)
	$(MAKE) $(MAKE_QUIET_FLAG) -C Userspace build
endif

build_image:
	mkdir -p iso_tmp_data
	mkdir -p tmp_rootfs
	cp -r rootfs/* tmp_rootfs/
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
	cp Kernel/build/fennix.elf rootfs.tar.gz iso_tmp_data/
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
		iso_tmp_data -o $(OSNAME).iso
endif
ifeq ($(BOOTLOADER), grub)
# TODO: Add custom language support for GRUB or detect the system language using "echo $LANG | cut -d . -f 1" and set "lang" variable inside grub.cfg
	mkdir -p iso_tmp_data/boot
	mkdir -p iso_tmp_data/boot/grub
	cp tools/grub.cfg iso_tmp_data/boot/grub/
	grub-mkrescue -d /usr/lib/grub/x86_64-efi -o $(OSNAME).iso iso_tmp_data
endif
ifneq ($(filter aarch64 arm,$(OSARCH)),)
	$(__CONF_OBJCOPY) Kernel/build/fennix.elf -O binary $(OSNAME).img
#	cp Bootloader/boot.bin $(OSNAME).img
endif

ifeq ($(OSARCH), amd64)
QEMU_UEFI_BIOS = -bios /usr/share/qemu/OVMF.fd
endif
# ifeq ($(OSARCH), aarch64)
# QEMU_UEFI_BIOS = -bios -bios /usr/share/AAVMF/AAVMF_CODE.fd
# endif

ifeq ($(OSARCH), amd64)
QEMU_SMP = -smp $(shell nproc)
endif

ifeq ($(OSARCH), i386)
QEMU_SMP = -smp $(shell nproc)
endif

ifeq ($(OSARCH), arm)
QEMU_SMP = -smp 4
endif

ifeq ($(OSARCH), aarch64)
QEMU_SMP = -smp 4
endif

ifeq ($(OSARCH), amd64)
QEMUHWACCELERATION = -machine q35 -enable-kvm
QEMUMEMORY = -m 4G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 256M
else ifeq ($(OSARCH), i386)
QEMUHWACCELERATION = -machine q35 -enable-kvm
QEMUMEMORY = -m 4G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 512M
else ifeq ($(OSARCH), arm)
QEMUHWACCELERATION =
QEMUMEMORY = -m 1G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 512M
else ifeq ($(OSARCH), aarch64)
QEMUHWACCELERATION =
QEMUMEMORY = -m 2G
QEMU_DBG_SMP = -smp 4
QEMU_DBG_MEMORY = -m 2G
endif

clean_logs:
	rm -f serial.log COM2.dmp COM3.dmp COM4.dmp \
		network.dmp \
		LPT1.dmp LPT2.dmp LPT3.dmp \
		mouse.pcap kbd.pcap

check_chmod_kvm:
ifeq ($(CHMOD_KVM), 1)
	sudo chmod 666 /dev/kvm
endif

vscode_debug_only: clean_logs check_chmod_kvm
ifneq ($(filter arm aarch64,$(OSARCH)),)
	$(QEMU) -S -chardev socket,path=/tmp/gdb-fennix,server=on,wait=off,id=gdb0 -gdb chardev:gdb0 \
	-d cpu_reset,int,unimp,guest_errors,mmu,fpu \
	-no-reboot -no-shutdown \
	$(QEMU_DBG_MEMORY) $(QEMUFLAGS) $(QEMU_DBG_SMP)
else
	$(QEMU) -S -chardev socket,path=/tmp/gdb-fennix,server=on,wait=off,id=gdb0 -gdb chardev:gdb0 \
	-d cpu_reset,int,unimp,guest_errors,mmu,fpu \
	-no-reboot -no-shutdown $(QEMU_UEFI_BIOS) \
	$(QEMU_DBG_MEMORY) $(QEMUFLAGS) $(QEMU_DBG_SMP)
endif

vscode_debug: build_kernel build_userspace build_drivers build_image vscode_debug_only

debug: vscode_debug

qemu: qemu_vdisk clean_logs check_chmod_kvm
	touch serial.log
#	x-terminal-emulator -e tail -f serial.log &
	$(QEMU) $(QEMU_UEFI_BIOS) -cpu host $(QEMUFLAGS) $(QEMUHWACCELERATION) $(QEMUMEMORY) $(QEMU_SMP)

qemubios: qemu_vdisk clean_logs check_chmod_kvm
	$(QEMU) -cpu host $(QEMUFLAGS) $(QEMUHWACCELERATION) $(QEMUMEMORY) $(QEMU_SMP)

run: build qemu

clean: clean_logs
	rm -rf doxygen-doc iso_tmp_data tmp_rootfs
	rm -f rootfs.tar.gz $(OSNAME).iso $(OSNAME).img
	$(MAKE) -C Kernel clean
	$(MAKE) -C Userspace clean
	$(MAKE) -C Drivers clean
	$(MAKE) -C Bootloader clean
