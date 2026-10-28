QEMU ?= qemu-system-x86_64
OSARCH ?= amd64

# For tap0
# -netdev tap,id=usernet0,ifname=tap0,script=no,downscript=no
QEMUFLAGS := -display gtk

ifeq ($(OSARCH), amd64)
QEMUFLAGS += -device vmware-svga -M q35 \
			 -cpu max \
			 \
			 -device piix3-usb-uhci,id=uhci \
			 -device usb-mouse,bus=uhci.0,pcap=tools/dump/mouse.pcap \
			 -device usb-kbd,bus=uhci.0,pcap=tools/dump/kbd.pcap \
			 -device usb-tablet,bus=uhci.0 \
			 -device usb-wacom-tablet,bus=uhci.0 \
			 -device usb-ccid,bus=uhci.0 \
			 \
			 -device pci-ohci,id=ohci \
			 -device usb-mouse,bus=ohci.0 \
			 -device usb-kbd,bus=ohci.0 \
			 -device usb-tablet,bus=ohci.0 \
			 -device usb-wacom-tablet,bus=ohci.0 \
			 -device usb-ccid,bus=ohci.0 \
			 \
			 -device usb-ehci,id=ehci \
			 -device usb-mouse,bus=ehci.0 \
			 -device usb-kbd,bus=ehci.0 \
			 \
			 -device qemu-xhci,id=xhci \
			 -device usb-mouse,bus=xhci.0 \
			 -device usb-kbd,bus=xhci.0 \
			 -device usb-tablet,bus=xhci.0 \
			 -device usb-wacom-tablet,bus=xhci.0 \
			 -device usb-ccid,bus=xhci.0 \
			 \
			 -net user \
			 -netdev user,id=usernet0 \
			 -device e1000,netdev=usernet0,mac=00:69:96:00:42:00 \
			 -object filter-dump,id=usernet0,netdev=usernet0,file=tools/dump/network.dmp,maxlen=1024 \
			 \
			 -trace usb_uhci_* \
			 -trace usb_ohci_* \
			 -trace usb_ehci_* \
			 -trace usb_xhci_* \
			 -trace usb_hub_* \
			 -trace usb_port_* \
			 -trace ps2_* \
			 -serial file:serial.log \
			 -serial file:tools/dump/COM2.dmp \
			 -serial file:tools/dump/COM3.dmp \
			 -serial stdio \
			 -parallel file:tools/dump/LPT1.dmp \
			 -parallel file:tools/dump/LPT2.dmp \
			 -parallel file:tools/dump/LPT3.dmp \
			 \
			 -device ahci,id=ahci \
			 -drive id=bootdsk,file=Fennix.iso,format=raw,if=none \
			 -device ide-hd,drive=bootdsk,bus=ahci.0 \
			 -drive id=disk,file=tools/qemu-disk.qcow2,if=none \
			 -device ide-hd,drive=disk,bus=ahci.1 \
			 \
			 -device isa-fdc \
			 -blockdev driver=file,node-name=f0,filename=tools/floppy0.img \
			 -device floppy,drive=f0,unit=0 \
			 -blockdev driver=file,node-name=f1,filename=tools/floppy1.img \
			 -device floppy,drive=f1,unit=1 \
			 \
			 -audiodev pa,id=pa1,server=/run/user/1000/pulse/native \
			 -machine pcspk-audiodev=pa1 \
			 -device AC97,audiodev=pa1 \
			 -device intel-hda \
			 -device ich9-intel-hda \
			 -acpitable file=tools/acpi/SSDT1.dat
else ifeq ($(OSARCH), i386)
QEMUFLAGS += -M q35 \
			 -cpu max \
			 -usb \
			 -device usb-tablet,bus=usb-bus.0,port=1 \
			 -device usb-hub,bus=usb-bus.0,port=2 \
			 -device qemu-xhci,id=xhci \
			 -device usb-mouse,bus=xhci.0 \
			 -device usb-kbd,bus=xhci.0 \
			 -device usb-mouse,pcap=tools/dump/mouse.pcap \
			 -device usb-kbd,pcap=tools/dump/kbd.pcap \
			 -net user \
			 -netdev user,id=usernet0 \
			 -device e1000,netdev=usernet0,mac=00:69:96:00:42:00 \
			 -object filter-dump,id=usernet0,netdev=usernet0,file=tools/dump/network.dmp,maxlen=1024 \
			 -serial file:serial.log \
			 -serial file:tools/dump/COM2.dmp \
			 -serial file:tools/dump/COM3.dmp \
			 -serial stdio \
			 -parallel file:tools/dump/LPT1.dmp \
			 -parallel file:tools/dump/LPT2.dmp \
			 -parallel file:tools/dump/LPT3.dmp \
			 -fda tools/floppy0.img -fdb tools/floppy1.img \
			 -hda Fennix.iso \
			 -audiodev pa,id=pa1,server=/run/user/1000/pulse/native \
			 -machine pcspk-audiodev=pa1 \
			 -device AC97,audiodev=pa1 \
			 -device intel-hda \
			 -device ich9-intel-hda \
			 -acpitable file=tools/acpi/SSDT1.dat
else ifeq ($(OSARCH), arm)
QEMUFLAGS += -M raspi2b \
			 -cpu cortex-a15 \
			 -serial file:serial.log \
			 -serial file:tools/dump/COM2.dmp \
			 -serial file:tools/dump/COM3.dmp \
			 -serial stdio \
			 -kernel Fennix.img
else ifeq ($(OSARCH), aarch64)
QEMUFLAGS += -M raspi4b \
			 -cpu cortex-a72 \
			 -serial file:serial.log \
			 -serial file:tools/dump/COM2.dmp \
			 -serial file:tools/dump/COM3.dmp \
			 -serial stdio \
			 -kernel Fennix.img
endif


ifeq ($(OSARCH), amd64)
QEMU_UEFI_BIOS = -bios /usr/share/qemu/OVMF.fd
endif
# ifeq ($(OSARCH), aarch64)
# QEMU_UEFI_BIOS = -bios -bios /usr/share/AAVMF/AAVMF_CODE.fd
# endif

ifeq ($(OSARCH), amd64)
QEMU_SMP = -smp $(shell nproc)
endif

ifeq ($(OSARCH), i386)
QEMU_SMP = -smp $(shell nproc)
endif

ifeq ($(OSARCH), arm)
QEMU_SMP = -smp 4
endif

ifeq ($(OSARCH), aarch64)
QEMU_SMP = -smp 4
endif

ifeq ($(OSARCH), amd64)
QEMUHWACCELERATION = -machine q35 -enable-kvm
QEMUMEMORY = -m 4G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 256M
else ifeq ($(OSARCH), i386)
QEMUHWACCELERATION = -machine q35 -enable-kvm
QEMUMEMORY = -m 4G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 512M
else ifeq ($(OSARCH), arm)
QEMUHWACCELERATION =
QEMUMEMORY = -m 1G
QEMU_DBG_SMP = -smp 1
QEMU_DBG_MEMORY = -m 512M
else ifeq ($(OSARCH), aarch64)
QEMUHWACCELERATION =
QEMUMEMORY = -m 2G
QEMU_DBG_SMP = -smp 4
QEMU_DBG_MEMORY = -m 2G
endif

.PHONY: default run dbg

default:
	$(error Do not run this file directly!)

check_chmod_kvm:
ifeq ($(USING_DEVCONTAINER), 1)
	sudo chmod 666 /dev/kvm
endif

vdisk:
	@test -e tools/qemu-disk.qcow2 || qemu-img create -f qcow2 tools/qemu-disk.qcow2 1G
	@test -e tools/floppy0.img || qemu-img create -f raw tools/floppy0.img 1440k
	@test -e tools/floppy1.img || qemu-img create -f raw tools/floppy1.img 1440k

run: vdisk check_chmod_kvm clean
	touch serial.log
#	x-terminal-emulator -e tail -f serial.log &
	$(QEMU) $(QEMU_UEFI_BIOS) -cpu host $(QEMUFLAGS) $(QEMUHWACCELERATION) $(QEMUMEMORY) $(QEMU_SMP)

bios: vdisk check_chmod_kvm clean
	$(QEMU) -cpu host $(QEMUFLAGS) $(QEMUHWACCELERATION) $(QEMUMEMORY) $(QEMU_SMP)

dbg: vdisk check_chmod_kvm clean
ifneq ($(filter arm aarch64,$(OSARCH)),)
	$(QEMU) -S -chardev socket,path=/tmp/gdb-fennix,server=on,wait=off,id=gdb0 -gdb chardev:gdb0 \
	-d cpu_reset,int,unimp,guest_errors,mmu,fpu \
	-no-reboot -no-shutdown \
	$(QEMU_DBG_MEMORY) $(QEMUFLAGS) $(QEMU_DBG_SMP)
else
	$(QEMU) -S -chardev socket,path=/tmp/gdb-fennix,server=on,wait=off,id=gdb0 -gdb chardev:gdb0 \
	-d cpu_reset,int,unimp,guest_errors,mmu,fpu \
	-no-reboot -no-shutdown $(QEMU_UEFI_BIOS) \
	$(QEMU_DBG_MEMORY) $(QEMUFLAGS) $(QEMU_DBG_SMP)
endif

clean:
	rm -f serial.log tools/dump/*.dmp tools/dump/*.pcap
