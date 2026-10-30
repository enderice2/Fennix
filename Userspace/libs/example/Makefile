default:
	$(error Do not run this Makefile directly!)

DYNAMIC_NAME := lib$(notdir $(shell pwd)).so
STATIC_NAME := lib$(notdir $(shell pwd)).a

OUTPUT_DIR=$(WORKSPACE_DIR)/out/usr/lib/
SYSROOT = --sysroot=$(WORKSPACE_DIR)/out/

S_SOURCES = $(shell find ./ -type f -name '*.S')
C_SOURCES = $(shell find ./ -type f -name '*.c')
CXX_SOURCES = $(shell find ./ -type f -name '*.cpp')
OBJ = ${S_SOURCES:.S=.o} ${C_SOURCES:.c=.o} ${CXX_SOURCES:.cpp=.o}

CFLAGS := -fvisibility=hidden -fPIC -I../include -I$(WORKSPACE_DIR)/out/usr/include -DLIBC_GIT_COMMIT='"$(shell git rev-parse HEAD)"'

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -ggdb3 -O0 -fdiagnostics-color=always
endif

build: $(DYNAMIC_NAME) $(STATIC_NAME)

.PHONY: $(DYNAMIC_NAME) $(STATIC_NAME)

$(DYNAMIC_NAME): $(OBJ)
	$(info Linking $@)
	$(CC) -nostdlib -shared -fPIC -fPIE -Wl,-soname,$(DYNAMIC_NAME) $(SYSROOT) $(OBJ) -o $(DYNAMIC_NAME)
	cp $(DYNAMIC_NAME) $(OUTPUT_DIR)$(DYNAMIC_NAME)

$(STATIC_NAME): $(OBJ)
	$(info Linking $@)
	$(AR) -rcs $(STATIC_NAME) $(OBJ)
	cp $(STATIC_NAME) $(OUTPUT_DIR)$(STATIC_NAME)

%.o: %.c
	$(info Compiling $<)
	$(CC) $(CFLAGS) -std=c17 -c $< -o $@

%.o: %.cpp
	$(info Compiling $<)
	$(CC) $(CFLAGS) -std=c++20 -c $< -o $@

%.o: %.S
	$(info Compiling $<)
	$(AS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DYNAMIC_NAME) $(STATIC_NAME)
