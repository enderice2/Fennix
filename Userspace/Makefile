export WORKSPACE_DIR = $(CURDIR)

ifeq ($(DEBUG), 1)

export LDFLAGS := --sysroot=$(WORKSPACE_DIR)/out/ \
	-ggdb3 -O0
export CFLAGS := \
	--sysroot=$(WORKSPACE_DIR)/out/ \
	-I$(WORKSPACE_DIR)/out/usr/include \
	-DDEBUG -ggdb3 -O0 -fdiagnostics-color=always

else # DEBUG

export LDFLAGS := --sysroot=$(WORKSPACE_DIR)/out/
export CFLAGS := \
	--sysroot=$(WORKSPACE_DIR)/out/ \
	-I$(WORKSPACE_DIR)/out/usr/include

endif # DEBUG

create_out:
	rm -rf out
	mkdir -p out/bin
	mkdir -p out/lib
	mkdir -p out/usr/bin
	mkdir -p out/usr/lib
	mkdir -p out/usr/include
	mkdir -p out/usr/share/doc
	mkdir -p out/usr/share/info
	cp $(WORKSPACE_DIR)/../Kernel/include/interface/* $(WORKSPACE_DIR)/out/usr/include/fennix/

build_coreutils:
	mkdir -p cache/coreutils
	cd cache/coreutils && \
	cmake $(WORKSPACE_DIR)/coreutils \
	-DCMAKE_INSTALL_PREFIX:PATH=$(WORKSPACE_DIR)/out \
	-DCMAKE_SYSROOT=$(WORKSPACE_DIR)/out \
	-DCMAKE_C_STANDARD_INCLUDE_DIRECTORIES=$(WORKSPACE_DIR)/out/usr/include \
	-DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=$(WORKSPACE_DIR)/out/usr/include \
	&& \
	$(MAKE) -j$(shell nproc) && \
	$(MAKE) install

define copy_generic_header
	cp -f $(WORKSPACE_DIR)/../Kernel/include/interface/$(1) $(WORKSPACE_DIR)/libc/abis/fennix/generic/bits/$(1)
endef

build_libc:
	$(call copy_generic_header,errno.h)
	$(call copy_generic_header,syscalls.h)
	$(call copy_generic_header,fcntl.h)
	mkdir -p cache/libc
	cd cache/libc && \
	cmake $(WORKSPACE_DIR)/libc \
	-DCMAKE_INSTALL_PREFIX:PATH=$(WORKSPACE_DIR)/out \
	-DCMAKE_SYSROOT=$(WORKSPACE_DIR)/out \
	-DCMAKE_C_STANDARD_INCLUDE_DIRECTORIES=$(WORKSPACE_DIR)/out/usr/include \
	-DTARGET_OS=fennix \
	-DTARGET_ARCH=$(OSARCH) \
	&& \
	$(MAKE) -j$(shell nproc) && \
	$(MAKE) install

build: create_out
	$(MAKE) build_libc
	$(MAKE) -C libs build
	$(MAKE) build_coreutils
	$(MAKE) -C apps build

prepare:
	$(MAKE) -C libs prepare

clean:
	rm -rf out cache
	mkdir -p cache
	touch cache/.gitkeep
	$(MAKE) -C libs clean
	$(MAKE) -C apps clean
	$(MAKE) -C docker clean
