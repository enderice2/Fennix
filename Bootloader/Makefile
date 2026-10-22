GNUEFI_RELEASE_VERSION=3.0.18

GIT_COMMIT = $(shell git rev-parse HEAD)
GIT_COMMIT_SHORT = $(shell git rev-parse --short HEAD)

export CC := $(__CONF_CC)
export CXX := $(__CONF_CXX)
export LD := $(__CONF_LD)
export AS := $(__CONF_AS)
export AR := $(__CONF_AR)
export NM := $(__CONF_NM)
export OBJCOPY := $(__CONF_OBJCOPY)
export OBJDUMP := $(__CONF_OBJDUMP)
export GDB := $(__CONF_GDB)

default:
	$(error Do not run this Makefile directly!)

prepare:
# ifeq ("$(wildcard ./gnu-efi)", "")
# 	wget https://freefr.dl.sourceforge.net/project/gnu-efi/gnu-efi-$(GNUEFI_RELEASE_VERSION).tar.bz2
# 	tar -xf gnu-efi-$(GNUEFI_RELEASE_VERSION).tar.bz2
# 	rm gnu-efi-$(GNUEFI_RELEASE_VERSION).tar.bz2
# 	mv ./gnu-efi-$(GNUEFI_RELEASE_VERSION) ./gnu-efi
# 	cp -a ./gnu-efi/inc/. ./include
# endif
#	echo "PREPARE > Compiling gnu-efi for x86_64"
#	make -C gnu-efi all ARCH=x86_64 \
#		AR=$(TOOLCHAIN_AMD64_PREFIX)ar \
#		AS=$(TOOLCHAIN_AMD64_PREFIX)as \
#		CC=$(TOOLCHAIN_AMD64_PREFIX)gcc \
#		LD=$(TOOLCHAIN_AMD64_PREFIX)ld
#	echo "PREPARE > Compiling gnu-efi for arm"
#	make -C gnu-efi all ARCH=arm \
#		AR=$(TOOLCHAIN_ARM_PREFIX)ar \
#		AS=$(TOOLCHAIN_ARM_PREFIX)as \
#		CC=$(TOOLCHAIN_ARM_PREFIX)gcc \
#		LD=$(TOOLCHAIN_ARM_PREFIX)ld \
#		CFLAGS="-I$(CURDIR)/gnu-efi/inc"
#	echo "PREPARE > Compiling gnu-efi for aarch64"
#	make -C gnu-efi all ARCH=aarch64 \
#		AR=$(TOOLCHAIN_AARCH64_PREFIX)ar \
#		AS=$(TOOLCHAIN_AARCH64_PREFIX)as \
#		CC=$(TOOLCHAIN_AARCH64_PREFIX)gcc \
#		LD=$(TOOLCHAIN_AARCH64_PREFIX)ld

build:
	$(MAKE) -C common build
	$(MAKE) -C firmware build
#	$(MAKE) -C uefi build

clean:
	$(MAKE) -C common clean
	$(MAKE) -C firmware clean
#	$(MAKE) -C uefi clean
	rm -f boot.bin
