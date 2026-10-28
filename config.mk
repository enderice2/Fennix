##############################################
#          ___              _                #
#         | __|__ _ _  _ _ (_)_ __           #
#         | _/ -_) ' \| ' \| \ \ /           #
#         |_|\___|_||_|_||_|_/_\_\           #
#                                            #
##############################################


# Enable debug mode
# This will make the build process more verbose and will also enable debug symbols in the binaries.
#
DEBUG = 1

# OS architecture
#
OSARCH = amd64

# This variable specifies the board type.
# It is crucial because the ARM booting process differs significantly from x86.
#
# ! This variable is ignored on x86 !
#
# Available options are in AVAILABLE_BOARDS.
#
BOARD_TYPE := raspi4

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
#   Default: /usr/local/bin
#
COMPILER_PATH := /usr/local/bin

# Qemu path.
#   Default: /usr/local/bin
#
QEMU_PATH := /usr/local/bin












# -----------------------------------------------------------------------------
# Make related variables
# -----------------------------------------------------------------------------

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

export TRIPLET := $(COMPILER_PATH)/$(COMPILER_ARCH)-fennix
export QEMU := $(QEMU_PATH)/qemu-system-$(COMPILER_ARCH)

export DEBUG
export OSARCH
export BOARD_TYPE
export QUIET_BUILD
export BOOTLOADER

export CC      := $(TRIPLET)-gcc
export CXX     := $(TRIPLET)-g++
export AS      := $(TRIPLET)-as
export LD      := $(CC)
export AR      := $(TRIPLET)-ar
export NM      := $(TRIPLET)-nm
export OBJCOPY := $(TRIPLET)-objcopy
export OBJDUMP := $(TRIPLET)-objdump

export KERNEL_ARCH := $(OSARCH)
export KERNEL_DEBUG := $(DEBUG)
