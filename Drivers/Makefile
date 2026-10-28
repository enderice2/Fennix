default:
	$(error Do not run this Makefile directly!)

export OUTPUT_DIR := $(CURDIR)/out/
export INCLUDE_DIR := $(CURDIR)/include

DRIVER_LDFLAGS = -nostdlib -nodefaultlibs -nolibc -zmax-page-size=0x1000	\
			-Wl,-Map file.map -fvisibility=hidden -Wl,--dynamic-linker=/boot/fennix.elf

ifeq ($(OSARCH), amd64)
DRIVER_CFLAGS = -fPIC -fPIE -pie -mno-80387 -mno-mmx -mno-3dnow	\
		  -mno-red-zone -mno-sse -mno-sse2							\
		  -march=x86-64 -pipe -ffunction-sections					\
		  -msoft-float -fno-builtin

else ifeq ($(OSARCH), i386)
DRIVER_CFLAGS = -fPIC -fPIE -pie -mno-80387 -mno-mmx -mno-3dnow	\
		  -mno-red-zone -mno-sse -mno-sse2 -ffunction-sections		\
		  -march=i386 -pipe -msoft-float -fno-builtin
else ifeq ($(OSARCH), aarch64)
DRIVER_CFLAGS = -pipe -fno-builtin -fPIC
endif

DRIVER_CFLAGS += -I$(CURDIR)/include

ifeq ($(DEBUG), 1)
	DRIVER_CFLAGS += -DDEBUG -ggdb3 -O0 -fdiagnostics-color=always -fstack-usage
ifeq ($(OSARCH), amd64)
	DRIVER_CFLAGS += -fverbose-asm
endif
ifneq ($(OSARCH), aarch64)
	DRIVER_CFLAGS += -fstack-check
endif
	DRIVER_LDFLAGS += -ggdb3 -O0
endif

export DRIVER_LDFLAGS
export DRIVER_CFLAGS

copy_driver_signatures:
	@TMP_FILE="$(OUTPUT_DIR)../../Kernel/drivers/trusted.c.tmp"; \
	OUT_FILE="$(OUTPUT_DIR)../../Kernel/drivers/trusted.c"; \
	mkdir -p $(OUTPUT_DIR)../../Kernel/drivers/; \
	echo "const char *trusted_drivers[] = {" > $$TMP_FILE; \
	find $(OUTPUT_DIR) -name "*.drv" -exec sha512sum {} \; | awk '{gsub(/.*\//, "", $$2); gsub(/\./, "_", $$2); sub(/_drv$$/, "_drv", $$2); print "\"" $$1 "\"," }' >> $$TMP_FILE; \
	echo "};" >> $$TMP_FILE; \
	echo "const __SIZE_TYPE__ trusted_drivers_count = sizeof(trusted_drivers) / sizeof(trusted_drivers[0]);" >> $$TMP_FILE; \
	if [ ! -f $$OUT_FILE ] || ! cmp -s $$TMP_FILE $$OUT_FILE; then \
		mv $$TMP_FILE $$OUT_FILE; \
		printf '\033[0;32m[trusted.c updated]\033[0m\n'; \
	else \
		rm $$TMP_FILE; \
		printf '\033[0;33m[trusted.c unchanged]\033[0m\n'; \
	fi

build:
	cp -rf ../Kernel/include/interface/* include/
	mkdir -p out
	$(MAKE) -C library build
ifneq ($(filter amd64 i386,$(OSARCH)),)
	$(MAKE) -C audio build
	$(MAKE) -C input build
	$(MAKE) -C misc build
	$(MAKE) -C network build
	$(MAKE) -C storage build
	$(MAKE) -C filesystem build
endif
	$(MAKE) copy_driver_signatures

prepare:
	$(info Nothing to prepare)

clean:
	rm -rf out
	$(MAKE) -C library clean
ifneq ($(filter amd64 i386,$(OSARCH)),)
	$(MAKE) -C audio clean
	$(MAKE) -C input clean
	$(MAKE) -C misc clean
	$(MAKE) -C network clean
	$(MAKE) -C storage clean
	$(MAKE) -C filesystem clean
endif
