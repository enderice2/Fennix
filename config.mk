# If the OS we are building should be compiled
# for debugging purposes.
DEBUG = 1

# Operating system name.
OSNAME = Fennix

# OS architecture, check AVAILABLE_ARCHS for available options.
OSARCH = amd64

# This variable specifies the board type.
# It is crucial because the ARM booting process differs significantly from x86.
#
# ! This variable is ignored on x86 !
#
# Available options are in AVAILABLE_BOARDS.
BOARD_TYPE := raspi4

# Kernel version.
KERNEL_VERSION = 0.0.1

# Which bootloader to use.
# Available bootloaders:
#   - builtin  - Built-in
#   - grub     - GRUB
#   - limine   - Limine
BOOTLOADER = grub

BUILD_KERNEL = 1
BUILD_BOOTLOADER = 0
BUILD_USERSPACE = 0
BUILD_DRIVERS = 0

QUIET_BUILD = 0

# The path of the cross-compiler.
#   Default: $(CURDIR)/tools/cross
COMPILER_PATH := $(CURDIR)/tools/cross

# Qemu path. If you want to use the one
# you have installed in your system, change
# it to /usr
# (do not include x86_64 or i386, it will be
# added automatically depending on the OSARCH)
#   Default: $(CURDIR)/tools/cross
__CONF_QEMU_PATH := $(CURDIR)/tools/cross










# Make related variables
# -----------------------
# Do not change anything below this line unless
# you know what you are doing.

# Available architectures. Do not change
export AVAILABLE_ARCHS := amd64 i386 arm aarch64

# Available board types. Do not change
export AVAILABLE_BOARDS := \
	raspi0 \
	raspi1 \
	raspi2 \
	raspi3 \
	raspi4

ifneq ($(filter $(OSARCH),$(AVAILABLE_ARCHS)),$(OSARCH))
$(error OSARCH=$(OSARCH) is not a supported architecture. Choose one of: $(AVAILABLE_ARCHS))
endif

ifeq ($(filter amd64 i386,$(OSARCH)),)
ifneq ($(filter $(BOARD_TYPE),$(AVAILABLE_BOARDS)),$(BOARD_TYPE))
$(error BOARD_TYPE=$(BOARD_TYPE) is not a supported board type. Choose one of: $(AVAILABLE_BOARDS))
endif
endif

ARCH_MAP := amd64=x86_64 i386=i386 arm=arm aarch64=aarch64
COMPILER_ARCH := $(patsubst $(OSARCH)=%,%,$(filter $(OSARCH)=%,$(ARCH_MAP)))
__CONF_QEMU_PATH := $(__CONF_QEMU_PATH)/bin/qemu-system-$(COMPILER_ARCH)
TOOLCHAIN_PREFIX := $(COMPILER_PATH)/bin/$(COMPILER_ARCH)-fennix-

export __CONF_CC := $(TOOLCHAIN_PREFIX)gcc
export __CONF_CXX := $(TOOLCHAIN_PREFIX)g++
export __CONF_LD := $(TOOLCHAIN_PREFIX)ld
export __CONF_AS := $(TOOLCHAIN_PREFIX)as
export __CONF_AR := $(TOOLCHAIN_PREFIX)ar
export __CONF_NM := $(TOOLCHAIN_PREFIX)nm
export __CONF_OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
export __CONF_OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
export __CONF_GDB := $(TOOLCHAIN_PREFIX)gdb

export DEBUG
export OSNAME
export OSARCH
export BOARD_TYPE
export KERNEL_VERSION
export QUIET_BUILD

export TOOLCHAIN_AMD64_PREFIX := $(COMPILER_PATH)/bin/x86_64-fennix-
export TOOLCHAIN_I386_PREFIX := $(COMPILER_PATH)/bin/i386-fennix-
export TOOLCHAIN_ARM_PREFIX := $(COMPILER_PATH)/bin/arm-fennix-
export TOOLCHAIN_AARCH64_PREFIX := $(COMPILER_PATH)/bin/aarch64-fennix-
