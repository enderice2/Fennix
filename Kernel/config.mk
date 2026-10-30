#################################################################################
#  ___              _       _  __                 _    ___           __ _       #
# | __|__ _ _  _ _ (_)_ __ | |/ /___ _ _ _ _  ___| |  / __|___ _ _  / _(_)__ _  #
# | _/ -_) ' \| ' \| \ \ / | ' </ -_) '_| ' \/ -_) | | (__/ _ \ ' \|  _| / _` | #
# |_|\___|_||_|_||_|_/_\_\ |_|\_\___|_| |_||_\___|_|  \___\___/_||_|_| |_\__, | #
#                                                                        |___/  #
#                                                                               #
#################################################################################


# Kernel version
# https://semver.org/
#
KERNEL_VERSION ?= 0.0.1

KERNEL_NAME     ?= Fennix
KERNEL_ARCH     ?= amd64
KERNEL_DEBUG    ?= 1

KERNEL_BUILD_TYPE ?= $(if $(filter 1,$(KERNEL_DEBUG)),debug,release)

KERNEL_FILENAME ?= fennix-$(KERNEL_VERSION)-$(KERNEL_ARCH)-$(KERNEL_BUILD_TYPE).elf
