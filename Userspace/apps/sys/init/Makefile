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

$(FILENAME).elf: $(OBJ)
	$(info Linking $@)
	$(CC) $(LDFLAGS) $(SYSROOT) $(OBJ) -o $@

%.o: %.c $(HEADERS)
	$(info Compiling $<)
	$(CC) $(CFLAGS) $(WARNCFLAG) -std=c17 -c $< -o $@

%.o: %.cpp $(HEADERS)
	$(info Compiling $<)
	$(CXX) $(CFLAGS) $(WARNCFLAG) -std=c++20 -fexceptions -c $< -o $@ -fno-rtti

%.o: %.S
	$(info Compiling $<)
	$(AS) -o $@ $<

clean:
	rm -f $(OBJ) $(FILENAME).elf
