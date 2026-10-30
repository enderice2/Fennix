default:
	$(error Do not run this Makefile directly!)

FILENAME = utest_

CFLAGS = -static -g -ggdb3 -O0 -w
UNAME_OS := $(shell uname -s)

$(FILENAME)linux:
ifneq ($(shell uname -s), Linux)
	$(info Only Linux is supported)
	exit 0
endif
	$(info Compiling $(FILENAME)linux)
	gcc linux_glibc.c $(CFLAGS) -o $(FILENAME)linux
	cp $(FILENAME)linux $(WORKSPACE_DIR)/out/bin/$(FILENAME)linux

$(FILENAME)win.exe:
ifneq ($(shell uname -s), Linux)
	$(info Only Linux is supported)
	exit 0
endif
	$(info Compiling $(FILENAME)win)
	x86_64-w64-mingw32-gcc win_mingw.c $(CFLAGS) -o $(FILENAME)win.exe
	cp $(FILENAME)win.exe $(WORKSPACE_DIR)/out/bin/$(FILENAME)win.exe

build: $(FILENAME)linux $(FILENAME)win.exe

clean:
	rm -f $(FILENAME)linux $(FILENAME)win.exe
