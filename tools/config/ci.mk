.PHONY: default

default:
	$(error Do not run this file directly!)

build-set-release:
	sed -i 's/.*DEBUG = .*/DEBUG = 0/' ./config.mk && cat config.mk | grep DEBUG

build-set-debug:
	sed -i 's/.*DEBUG = .*/DEBUG = 1/' ./config.mk && cat config.mk | grep DEBUG

amd64-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-amd64-debug.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

amd64-release:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-amd64-release.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

i386-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = i386/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-i386-debug.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

i386-release:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = i386/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-i386-release.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

arm-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = arm/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-arm-debug.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

arm-release:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = arm/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-arm-release.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

aarch64-debug:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = aarch64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-aarch64-debug.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

aarch64-release:
	$(MAKE) -C tools do_limine
	$(MAKE) build-set-release
	sed -i 's/.*OSARCH = .*/OSARCH = aarch64/' ./config.mk && cat config.mk | grep OSARCH
	$(MAKE) build
	mv Fennix.iso Fennix-aarch64-release.iso
	cp Kernel/build/fennix-*.elf .
	$(MAKE) clean

restore-config:
	$(MAKE) build-set-debug
	sed -i 's/.*OSARCH = .*/OSARCH = amd64/' ./config.mk && cat config.mk | grep OSARCH

prepare-archive:
	mkdir -p artifacts
	$(MAKE) changelog
	cp -f CHANGELOG.md artifacts/
	mv -f Fennix-*.iso artifacts/
	mv -f fennix-*.elf artifacts/
	sha256sum artifacts/Fennix-*.iso artifacts/fennix-*.elf > artifacts/SHA256SUMS
