default:
	$(error Do not run this Makefile directly!)

S_SOURCES = $(shell find ./ -type f -name '*.S')
C_SOURCES = $(shell find ./ -type f -name '*.c')
CXX_SOURCES = $(shell find ./ -type f -name '*.cpp')

OBJ = $(S_SOURCES:.S=.o) $(C_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o)

FILENAME = $(notdir $(shell pwd))
WARNCFLAG = -Wall -Wextra

build: $(FILENAME).elf
	cp $(FILENAME).elf $(WORKSPACE_DIR)/out/bin/$(FILENAME)

# Use static linking
LDFLAGS += -ggdb3 -g -static -fno-pic -fno-pie -Wl,-static -Wl,-Ttext=0x00600000
CFLAGS += -ggdb3 -g -O0 -fdiagnostics-color=always

# Use default toolchain if not specified
CC ?= gcc
CXX ?= g++
AS ?= as
GDBSERVER ?= /usr/bin/gdbserver

.PHONY: build_native run_native debug_native log_docker

log_docker:
#	docker compose logs -f libc_test

debug_native:
	$(MAKE) --quiet clean
	$(MAKE) --quiet -j $(shell nproc) build_native
	$(GDBSERVER) 0.0.0.0:9229 $(CURDIR)/$(FILENAME)_n.elf

build_native: $(FILENAME)_n.elf

run_native:
	$(MAKE) --quiet -j $(shell nproc) build_native
	./$(FILENAME)_n.elf

$(FILENAME)_n.elf $(FILENAME).elf: $(OBJ)
	$(info Linking $@)
	$(CC) $(LDFLAGS) $(SYSROOT) $(OBJ) -o $@ -lm

%.o: %.c $(HEADERS)
	$(info Compiling $<)
	$(CC) $(CFLAGS) $(WARNCFLAG) -std=c17 -c $< -o $@ -lm

%.o: %.cpp $(HEADERS)
	$(info Compiling $<)
	$(CXX) $(CFLAGS) $(WARNCFLAG) -std=c++20 -fexceptions -c $< -o $@ -fno-rtti

%.o: %.S
	$(info Compiling $<)
	$(AS) -o $@ $<

clean:
	rm -f $(OBJ) $(FILENAME).elf $(FILENAME)_n.elf
