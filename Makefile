
include config.mk

.PHONY: default tools clean ci-build build run

# First rule
default:
	$(error Please specify a target)

ifeq ($(QUIET_BUILD), 1)
MAKE_QUIET_FLAG := --quiet
endif

doxygen:
	mkdir -p doxygen-doc
	doxygen tools/Doxyfile

changelog:
	git cliff > CHANGELOG.md

# Install necessary packages, build cross-compiler etc...
tools:
	$(MAKE) --quiet -C tools all

dump:
	$(MAKE) --quiet -C Kernel dump

prepare:
	$(MAKE) mkdir_rootfs
	$(MAKE) --quiet -C Kernel prepare
	$(MAKE) --quiet -C Bootloader prepare
	$(MAKE) --quiet -C Drivers prepare
	$(MAKE) --quiet -C Userspace prepare

MKDIR_ROOTFS = mkdir -p tools/rootfs
mkdir_rootfs:
	$(MKDIR_ROOTFS)/home/root/desktop/
	$(MKDIR_ROOTFS)/home/root/documents/
	$(MKDIR_ROOTFS)/home/root/downloads/
	$(MKDIR_ROOTFS)/home/root/music/
	$(MKDIR_ROOTFS)/home/root/pictures/
	$(MKDIR_ROOTFS)/home/root/videos/
	$(MKDIR_ROOTFS)/home/root/templates/

	$(MKDIR_ROOTFS)/bin/
	$(MKDIR_ROOTFS)/etc/
	$(MKDIR_ROOTFS)/lib/modules/
	$(MKDIR_ROOTFS)/var/log/panic/

	$(MKDIR_ROOTFS)/usr/bin/
	$(MKDIR_ROOTFS)/usr/lib/
	$(MKDIR_ROOTFS)/usr/include/
	$(MKDIR_ROOTFS)/usr/share/subsystem/

	$(MKDIR_ROOTFS)/mnt/
	$(MKDIR_ROOTFS)/tmp/

setup:
	$(MAKE) prepare
ifeq ($(USING_DEVCONTAINER), 1)
	$(MAKE) --quiet -C tools do_limine
else
	$(MAKE) --quiet -C tools all
endif

build:
	$(MAKE) $(MAKE_QUIET_FLAG) build_kernel
	$(MAKE) $(MAKE_QUIET_FLAG) build_bootloader
	$(MAKE) $(MAKE_QUIET_FLAG) build_userspace
	$(MAKE) $(MAKE_QUIET_FLAG) build_drivers
	$(MAKE) $(MAKE_QUIET_FLAG) build_image

__ci-amd64-debug:
	$(MAKE) -f tools/config/ci.mk amd64-debug

__ci-amd64-release:
	$(MAKE) -f tools/config/ci.mk amd64-release

__ci-i386-debug:
	$(MAKE) -f tools/config/ci.mk i386-debug

__ci-i386-release:
	$(MAKE) -f tools/config/ci.mk i386-release

__ci-arm-debug:
	$(MAKE) -f tools/config/ci.mk arm-debug

__ci-arm-release:
	$(MAKE) -f tools/config/ci.mk arm-release

__ci-aarch64-debug:
	$(MAKE) -f tools/config/ci.mk aarch64-debug

__ci-aarch64-release:
	$(MAKE) -f tools/config/ci.mk aarch64-release

__ci-restore-config:
	$(MAKE) -f tools/config/ci.mk restore-config

__ci-prepare-archive:
	$(MAKE) -f tools/config/ci.mk prepare-archive

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
	$(MAKE) -f tools/config/image.mk build

vscode_debug_only:
	$(MAKE) -f tools/config/qemu.mk dbg

vscode_debug: build_kernel build_userspace build_drivers build_image vscode_debug_only

debug: vscode_debug

qemu:
	$(MAKE) -f tools/config/qemu.mk run

qemubios:
	$(MAKE) -f tools/config/qemu.mk bios

run: build qemu

clean:
	rm -rf doxygen-doc iso_tmp_data tmp_rootfs
	rm -f rootfs.tar.gz Fennix.iso Fennix.img
	$(MAKE) -C Kernel clean
	$(MAKE) -C Userspace clean
	$(MAKE) -C Drivers clean
	$(MAKE) -C Bootloader clean
	$(MAKE) -f tools/config/qemu.mk clean
