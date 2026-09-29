WORKING_DIR = $(CURDIR)
# CROSS_DIR=$(WORKING_DIR)/cross
CROSS_DIR=/usr/local
export PATH := $(CROSS_DIR):$(PATH)

BINUTILS_TAG = binutils-2_43_1
GCC_TAG = releases/gcc-14.2.0
QEMU_TAG = v9.2.0

QEMU_CLONE_PATH = /tmp/qemu
GCC_CLONE_PATH = /tmp/gcc
BINUTILS_CLONE_PATH = /tmp/binutils-gdb

default:
	$(error Please specify a target)

all: do_tools do_limine __clone_qemu __clone_all_no_qemu do_binutils do_gcc do_qemu
ci: do_tools do_limine __clone_all_no_qemu do_binutils do_gcc

clean:
	rm -f rep ep
	rm -rf $(BINUTILS_CLONE_PATH)
	rm -rf $(GCC_CLONE_PATH)
	rm -rf $(QEMU_CLONE_PATH)

do_tools:
	gcc -w ReadEthernetPackets.c -o rep
	g++ -w ErrorParser.cpp -o ep
	chmod +x rep
	chmod +x ep

do_limine:
ifeq ("$(wildcard ./limine)", "")
	git clone https://github.com/limine-bootloader/limine.git --branch=v9.x-binary --depth=1 limine
else
	$(info > TOOLS: Skipping cloning limine because directory already exists.)
endif

__clone_qemu:
ifeq ("$(wildcard $(QEMU_CLONE_PATH))", "")
	git clone --depth 1 -b ${QEMU_TAG} --single-branch https://gitlab.com/qemu-project/qemu.git $(QEMU_CLONE_PATH)
else
	$(info > TOOLS: Reseting qemu...)
	cd $(QEMU_CLONE_PATH) && \
	git fetch origin && \
	git reset --hard ${QEMU_TAG} && \
	git clean -dfx
	$(info > TOOLS: Operation completed for qemu)
endif

.PHONY: all clean __clone_binutils __clone_gcc __prep_cross __patch_cross __patch_cross_binutils __patch_cross_gcc

__clone_binutils:
ifeq ("$(wildcard $(BINUTILS_CLONE_PATH))", "")
	git clone --depth 1 -b $(BINUTILS_TAG) --single-branch git://sourceware.org/git/binutils-gdb.git $(BINUTILS_CLONE_PATH)
else
	$(info > TOOLS: Reseting binutils-gdb...)
	cd $(BINUTILS_CLONE_PATH) && \
	git fetch origin && \
	git reset --hard $(BINUTILS_TAG) && \
	git clean -dfx
	$(info > TOOLS: Operation completed for binutils-gdb)
endif

__clone_gcc:
ifeq ("$(wildcard $(GCC_CLONE_PATH))", "")
	git clone --depth 1 -b $(GCC_TAG) --single-branch git://gcc.gnu.org/git/gcc.git $(GCC_CLONE_PATH)
else
	$(info > TOOLS: Reseting gcc...)
	cd $(GCC_CLONE_PATH) && \
	git fetch origin && \
	git reset --hard $(GCC_TAG) && \
	git clean -dfx
	$(info > TOOLS: Operation completed for gcc)
endif

__patch_cross_binutils: __clone_binutils
	$(info > TOOLS: Patching binutils-gcc)
	cd $(BINUTILS_CLONE_PATH) && git apply $(CURDIR)/binutils-gdb.patch
	$(info > TOOLS: Running automake for binutils-gdb/ld)
	cd $(BINUTILS_CLONE_PATH)/ld && automake

__patch_cross_gcc: __clone_gcc
	$(info > TOOLS: Patching gcc)
	cd $(GCC_CLONE_PATH) && git apply $(CURDIR)/gcc.patch
	cd $(GCC_CLONE_PATH) && ./contrib/download_prerequisites
	$(info > TOOLS: Running autoconf for gcc/libstdc++-v3)
	cd $(GCC_CLONE_PATH)/libstdc++-v3 && autoconf

__patch_cross_qemu: __clone_qemu
	$(info > TOOLS: Patching qemu)
	cd $(QEMU_CLONE_PATH) && git apply $(CURDIR)/qemu.patch

__patch_cross: __patch_cross_binutils __patch_cross_gcc

__prep_cross:
	mkdir -p cross
	mkdir -p $(BINUTILS_CLONE_PATH)/__build
	mkdir -p $(GCC_CLONE_PATH)/__build

__clone_all_no_qemu: __clone_binutils __clone_gcc

cross_dev:
	$(MAKE) __clone_binutils
	$(MAKE) __clone_gcc
	$(MAKE) __clone_qemu
	$(MAKE) __patch_cross_binutils
	$(MAKE) __patch_cross_gcc
	$(MAKE) __patch_cross_qemu

update_patches:
ifeq ("$(wildcard ./binutils-gdb.patch.bk)", "")
	mv binutils-gdb.patch binutils-gdb.patch.bk
endif
ifeq ("$(wildcard ./gcc.patch.bk)", "")
	mv gcc.patch gcc.patch.bk
endif
ifeq ("$(wildcard ./qemu.patch.bk)", "")
	mv qemu.patch qemu.patch.bk
endif
# binutils-gdb
	cd $(BINUTILS_CLONE_PATH) && \
	git add . && \
	git diff --cached > $(CURDIR)/binutils-gdb.patch
# gcc
	cd $(GCC_CLONE_PATH) && \
	git add . && \
	git diff --cached > $(CURDIR)/gcc.patch
# qemu
	cd $(QEMU_CLONE_PATH) && \
	git add . && \
	git diff --cached > $(CURDIR)/qemu.patch

__do_binutils:
	mkdir -p $(BINUTILS_CLONE_PATH)/__build
	$(MAKE) __patch_cross_binutils
	$(MAKE) __prep_cross
	cd $(BINUTILS_CLONE_PATH)/__build && \
	../configure --target=$(BUILD_TARGET) --prefix="$(CROSS_DIR)" --quiet \
	--disable-nls \
	--with-sysroot \
	--disable-werror \
	--disable-shared \
	--enable-static \
	--enable-static-link \
	--disable-shared-plugins \
	--disable-dynamicplugin \
	--disable-tls \
	--disable-pie \
	&& \
	$(MAKE) --quiet all -j$(shell nproc) && \
	sudo $(MAKE) --quiet install

__do_gcc:
	mkdir -p $(GCC_CLONE_PATH)/__build
	$(MAKE) __patch_cross_gcc
	$(MAKE) __prep_cross
	cd $(GCC_CLONE_PATH)/__build && \
	../configure --target=$(BUILD_TARGET) --prefix="$(CROSS_DIR)" --quiet \
	--disable-nls \
	--enable-default-pie \
	--enable-languages=c,c++ \
	--without-headers \
	--disable-shared \
	--enable-static \
	--enable-static-link \
	--disable-tls \
	--disable-pie \
	--enable-libada \
	--enable-libgm2 \
	--enable-libssp \
	--enable-libstdcxx \
	--disable-hosted-libstdcxx \
	&& \
	$(MAKE) --quiet all-gcc -j$(shell nproc) && \
	$(MAKE) --quiet all-target-libgcc -j$(shell nproc) && \
	$(MAKE) --quiet all-target-libstdc++-v3 -j$(shell nproc) && \
	sudo $(MAKE) --quiet install-gcc -j$(shell nproc) && \
	sudo $(MAKE) --quiet install-target-libgcc -j$(shell nproc) && \
	sudo $(MAKE) --quiet install-target-libstdc++-v3 -j$(shell nproc)

do_binutils:
	$(MAKE) __do_binutils BUILD_TARGET="x86_64-fennix"
	$(MAKE) __do_binutils BUILD_TARGET="i386-fennix"
	$(MAKE) __do_binutils BUILD_TARGET="arm-fennix"
	$(MAKE) __do_binutils BUILD_TARGET="aarch64-fennix"

do_gcc:
	$(MAKE) __do_gcc BUILD_TARGET="x86_64-fennix"
	$(MAKE) __do_gcc BUILD_TARGET="i386-fennix"
	$(MAKE) __do_gcc BUILD_TARGET="arm-fennix"
	$(MAKE) __do_gcc BUILD_TARGET="aarch64-fennix"

do_qemu:
	$(MAKE) __patch_cross_qemu
	$(MAKE) __prep_cross
	cd $(QEMU_CLONE_PATH) && \
	bash ./configure --target-list=x86_64-softmmu,i386-softmmu,arm-softmmu,aarch64-softmmu \
	--prefix="$(CROSS_DIR)" \
	--enable-gtk \
	--disable-tools \
	--disable-gio \
	--disable-virtfs \
	--disable-vnc \
	--disable-opengl \
	&& \
	$(MAKE) --quiet -j$(shell nproc) && \
	sudo $(MAKE) --quiet install
