# -----------------------------------------------------------------------------
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
# https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html
# -----------------------------------------------------------------------------

KERNEL_FILENAME ?= fennix.elf

KERNEL_NAME ?= Fennix
KERNEL_VERSION ?= 0.0.0
KERNEL_ARCH ?= amd64
KERNEL_DEBUG  ?= 1

BUILD_DIR ?= build
OBJROOT   := $(BUILD_DIR)/obj
DEPROOT   := $(BUILD_DIR)/dep
OUTROOT   := $(BUILD_DIR)

GIT_COMMIT ?= $(shell git rev-parse HEAD 2>/dev/null || echo unknown)
GIT_COMMIT_SHORT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------

TRIPLET_amd64 ?= x86_64-fennix
TRIPLET_i386  ?= i386-fennix
TRIPLET_arm64 ?= aarch64-fennix

TRIPLET ?= $(TRIPLET_$(KERNEL_ARCH))

CC      := $(TRIPLET)-gcc
CXX     := $(TRIPLET)-g++
AS      := $(TRIPLET)-as
LD      := $(CC)
AR      := $(TRIPLET)-ar
NM      := $(TRIPLET)-nm
OBJCOPY := $(TRIPLET)-objcopy
OBJDUMP := $(TRIPLET)-objdump

# -----------------------------------------------------------------------------
# Arch layout
# -----------------------------------------------------------------------------

ARCH_FAMILY_amd64 := x86
ARCH_FAMILY_i386  := x86
ARCH_FAMILY_arm64 := arm64

ARCH_VARIANT_amd64 := amd64
ARCH_VARIANT_i386  := i386
ARCH_VARIANT_arm64 := generic

ARCH_FAMILY  := $(ARCH_FAMILY_$(KERNEL_ARCH))
ARCH_VARIANT := $(ARCH_VARIANT_$(KERNEL_ARCH))

ifeq ($(strip $(ARCH_FAMILY)),)
$(error Unsupported KERNEL_ARCH="$(KERNEL_ARCH)")
endif

ARCH_DIR        := arch/$(ARCH_FAMILY)/$(ARCH_VARIANT)
ARCH_COMMON_DIR := arch/$(ARCH_FAMILY)/common
LINKER_SCRIPT   := $(ARCH_DIR)/linker.ld

ifeq ($(wildcard $(LINKER_SCRIPT)),)
$(error Linker script "$(LINKER_SCRIPT)" does not exist)
endif

# -----------------------------------------------------------------------------
# Source discovery
# -----------------------------------------------------------------------------

SRC_ROOT := .

EXCLUDE_DIRS := \
	./build \
	./arch \
	./.vscode \
	./include \
	./include_std

FIND_PRUNE := $(foreach d,$(EXCLUDE_DIRS),-path $(d) -prune -o)

define find-src
$(shell find $(SRC_ROOT) \( $(FIND_PRUNE) -false \) -o -type f -name '$1' -print)
endef

SRC_DIRS_RAW := $(ARCH_COMMON_DIR) $(ARCH_DIR)
SRC_DIRS     := $(filter-out ,$(foreach d,$(SRC_DIRS_RAW),$(if $(wildcard $(d)),$(d))))

C_SOURCES   := $(ROOT_C_SOURCES)   $(call find-src,*.c)   $(shell find $(SRC_DIRS) -type f -name '*.c'   2>/dev/null)
CPP_SOURCES := $(ROOT_CPP_SOURCES) $(call find-src,*.cpp) $(shell find $(SRC_DIRS) -type f -name '*.cpp' 2>/dev/null)
S_SOURCES   := $(ROOT_S_SOURCES)   $(call find-src,*.S)   $(shell find $(SRC_DIRS) -type f -name '*.S'   2>/dev/null)
s_SOURCES   := $(ROOT_s_SOURCES)   $(call find-src,*.s)   $(shell find $(SRC_DIRS) -type f -name '*.s'   2>/dev/null)
PNG_SOURCES := $(ROOT_PNG_SOURCES) $(call find-src,*.png) $(shell find $(SRC_DIRS) -type f -name '*.png' 2>/dev/null)
PSF_SOURCES := $(ROOT_PSF_SOURCES) $(call find-src,*.psf) $(shell find $(SRC_DIRS) -type f -name '*.psf' 2>/dev/null)

C_OBJS   := $(addprefix $(OBJROOT)/,$(C_SOURCES:.c=.o))
CPP_OBJS := $(addprefix $(OBJROOT)/,$(CPP_SOURCES:.cpp=.o))
S_OBJS   := $(addprefix $(OBJROOT)/,$(S_SOURCES:.S=.o))
s_OBJS   := $(addprefix $(OBJROOT)/,$(s_SOURCES:.s=.o))
PNG_OBJS := $(addprefix $(OBJROOT)/,$(PNG_SOURCES:.png=.o))
PSF_OBJS := $(addprefix $(OBJROOT)/,$(PSF_SOURCES:.psf=.o))

OBJS := $(C_OBJS) $(CPP_OBJS) $(S_OBJS) $(s_OBJS) $(PNG_OBJS) $(PSF_OBJS)
DEPS := $(patsubst $(OBJROOT)/%.o,$(DEPROOT)/%.d,$(C_OBJS) $(CPP_OBJS) $(S_OBJS))

# -----------------------------------------------------------------------------
# Flags
# -----------------------------------------------------------------------------

CPPFLAGS += \
	-Iinclude \
	-Iinclude_std \
	-I$(ARCH_DIR)/include \
	-D__kernel__=1 \
	-DKERNEL_NAME='"$(KERNEL_NAME)"' \
	-DKERNEL_ARCH='"$(KERNEL_ARCH)"' \
	-DKERNEL_VERSION='"$(KERNEL_VERSION)"' \
	-DGIT_COMMIT='"$(GIT_COMMIT)"' \
	-DGIT_COMMIT_SHORT='"$(GIT_COMMIT_SHORT)"'

ifneq ($(wildcard $(ARCH_COMMON_DIR)/include),)
CPPFLAGS += -I$(ARCH_COMMON_DIR)/include
endif

CFLAGS += \
	-ffreestanding \
	-fno-builtin \
	-fno-pic \
	-fno-pie

CXXFLAGS += \
	$(CFLAGS) \
	-fno-rtti \
	-std=c++20

WARNFLAGS := \
	-Wall \
	-Wextra \
	-Wfloat-equal \
	-Wpointer-arith \
	-Wcast-align \
	-Wredundant-decls \
	-Winit-self \
	-Wswitch-default \
	-Wstrict-overflow=5 \
	-Wno-error=cpp \
	-Werror \
	-Wno-unused-parameter \
	-Wno-error=format

CFLAGS   += $(WARNFLAGS)
CXXFLAGS += $(WARNFLAGS)

CFLAG_STACK_PROTECTOR := -fstack-protector-all

LDFLAGS += \
	-static \
	-nostdlib \
	-nodefaultlibs \
	-nolibc \
	-Wl,-static,--no-dynamic-linker,-ztext \
	-zmax-page-size=0x1000 \
	-Wl,-Map,$(OUTROOT)/kernel.map

# Architecture-specific tuning
ifeq ($(KERNEL_ARCH),amd64)
CFLAGS   += -march=core2 -mcmodel=kernel -m64 -mno-red-zone
CXXFLAGS += -march=core2 -mcmodel=kernel -m64 -mno-red-zone
LDFLAGS  += -T$(LINKER_SCRIPT)
BINFMT := elf64-x86-64
else ifeq ($(KERNEL_ARCH),i386)
CFLAGS   += -march=pentium -m32
CXXFLAGS += -march=pentium -m32
LDFLAGS  += -T$(LINKER_SCRIPT)
BINFMT := elf32-i386
else ifeq ($(KERNEL_ARCH),arm64)
CFLAGS   += -mcmodel=large -mlittle-endian
CXXFLAGS += -mcmodel=large -mlittle-endian
LDFLAGS  += -T$(LINKER_SCRIPT)
BINFMT := elf64-littleaarch64
else
$(error Unsupported KERNEL_ARCH="$(KERNEL_ARCH)")
endif

ifeq ($(KERNEL_DEBUG),1)
GLOB_CFLAGS += -DDEBUG -ggdb3 -O0 -fdiagnostics-color=always -fstack-usage -Wno-unused-variable
CFLAGS   += $(GLOB_CFLAGS)
CXXFLAGS += $(GLOB_CFLAGS)
LDFLAGS  += -ggdb3 -O0
ASFLAGS  += -g --gdwarf-5
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: build kernel dump clean info

build: kernel

kernel: $(OUTROOT)/$(KERNEL_FILENAME)

info:
	@echo "KERNEL_NAME    = $(KERNEL_NAME)"
	@echo "KERNEL_VERSION = $(KERNEL_VERSION)"
	@echo "KERNEL_ARCH    = $(KERNEL_ARCH)"
	@echo "KERNEL_DEBUG   = $(KERNEL_DEBUG)"
	@echo "ARCH_DIR       = $(ARCH_DIR)"
	@echo "BUILD_DIR      = $(BUILD_DIR)"
	@echo "CC             = $(CC)"
	@echo "CXX            = $(CXX)"
	@echo "AS             = $(AS)"
	@echo "LD             = $(LD)"

$(OUTROOT):
	@mkdir -p $@

$(OUTROOT)/$(KERNEL_FILENAME): $(OBJS) | $(OUTROOT)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(OBJROOT)/%.o: %.c
	@mkdir -p $(dir $@) $(dir $(DEPROOT)/$*)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAG_STACK_PROTECTOR) -MMD -MP -MF $(DEPROOT)/$*.d -MT $@ -std=c17 -c $< -o $@

$(OBJROOT)/%.o: %.cpp
	@mkdir -p $(dir $@) $(dir $(DEPROOT)/$*)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CFLAG_STACK_PROTECTOR) -MMD -MP -MF $(DEPROOT)/$*.d -MT $@ -c $< -o $@

$(OBJROOT)/%.o: %.S
	@mkdir -p $(dir $@) $(dir $(DEPROOT)/$*)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASFLAGS) -MMD -MP -MF $(DEPROOT)/$*.d -MT $@ -c $< -o $@

$(OBJROOT)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(OBJROOT)/%.o: %.png
	@mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O $(BINFMT) $< $@

$(OBJROOT)/%.o: %.psf
	@mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O $(BINFMT) $< $@

# Dependency files
-include $(DEPS)

dump: $(OUTROOT)/$(KERNEL_FILENAME)
	$(OBJDUMP) -D -g -s -d $< > $(OUTROOT)/kernel_dump_att.map
	$(OBJDUMP) -M intel -D -g -s -d $< > $(OUTROOT)/kernel_dump_intel.map

clean:
	rm -rf $(BUILD_DIR)
