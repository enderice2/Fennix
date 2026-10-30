/*
	This file is part of Fennix Kernel.

	Fennix Kernel is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix Kernel is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix Kernel. If not, see <https://www.gnu.org/licenses/>.
*/

#include "kernel.h"

#include <fs/ustar.hpp>
#include <memory.hpp>
#include <convert.h>
#include <irq.hpp>
#include <printf.h>
#include <lock.hpp>
#include <kcon.hpp>
#include <debug.h>
#include <smp.hpp>
#include <cargs.h>
#include <io.h>

#include "tests/t.h"

bool DebuggerIsAttached = false;
NewLock(KernelLock);

__aligned(16) BootInfo bInfo{};

struct KernelConfig Config = {
	.AllocatorType = Memory::liballoc11,
	.SchedulerType = Multi,
	.DriverDirectory = {'/', 'l', 'i', 'b', '/', 'm', 'o', 'd', 'u', 'l', 'e', 's', '\0'},
	.InitPath = {'/', 'b', 'i', 'n', '/', 'i', 'n', 'i', 't', '\0'},
	.LinuxSubsystem = false,
	.InterruptsOnCrash = true,
	.Cores = 0,
	.IOAPICInterruptCore = 0,
	.UnlockDeadLock = false,
	.SIMD = true,
	.Quiet = false,
};

Interrupt::Manager irq;
UART::Driver uart;
Video::Display *Display = nullptr;
SymbolResolver::Symbols *KernelSymbolTable = nullptr;
Power::Power *PowerManager = nullptr;
Tasking::Task *TaskManager = nullptr;
PCI::Manager *PCIManager = nullptr;
Driver::Manager *DriverManager = nullptr;
UniversalSerialBus::Manager *usb = nullptr;

void main_thread();
void KernelVFS();

int main()
{
	Display = new Video::Display(bInfo.Framebuffer[0], false);
	KernelConsole::EarlyInit();
	printf("\x1b[H\x1b[2J");

	klog("%s - %s [\x1b[32m%s\x1b[0m]", KERNEL_NAME, KERNEL_VERSION, GIT_COMMIT_SHORT);

	if (Display->GetFramebufferStruct().BitsPerPixel != 32)
		warn("\x1b[1;31mFramebuffer is not 32 bpp. This may cause issues.");

	if (Display->GetWidth < 640 || Display->GetHeight < 480)
	{
		warn("\x1b[1;31mMinimum supported resolution is 640x480!");
		warn("\x1b[1;31mSome elements may not be displayed correctly.");
	}

#if defined(__amd64__) || defined(__i386__) && defined(DEBUG)
	debug("CPU: %s %s %s", CPU::Hypervisor(), CPU::Vendor(), CPU::Name());

	uint8_t lpt1 = inb(0x378);
	uint8_t lpt2 = inb(0x278);
	uint8_t lpt3 = inb(0x3BC);

	uint8_t com1 = inb(0x3F8);
	uint8_t com2 = inb(0x2F8);
	uint8_t com3 = inb(0x3E8);
	uint8_t com4 = inb(0x2E8);
	uint8_t com5 = inb(0x5F8);
	uint8_t com6 = inb(0x4F8);
	uint8_t com7 = inb(0x5E8);
	uint8_t com8 = inb(0x4E8);

	if (lpt1 != 0xFF)
		klog("LPT1 is present.");

	if (lpt2 != 0xFF)
		klog("LPT2 is present.");

	if (lpt3 != 0xFF)
		klog("LPT3 is present.");

	if (com1 != 0xFF)
		klog("COM1 is present.");

	if (com2 != 0xFF)
		klog("COM2 is present.");

	if (com3 != 0xFF)
		klog("COM3 is present.");

	if (com4 != 0xFF)
		klog("COM4 is present.");

	if (com5 != 0xFF)
		klog("COM5 is present.");

	if (com6 != 0xFF)
		klog("COM6 is present.");

	if (com7 != 0xFF)
		klog("COM7 is present.");

	if (com8 != 0xFF)
		klog("COM8 is present.");

	klog("Display: %dx%d %d bpp R:%d %d G:%d %d B:%d %d",
		 Display->GetFramebufferStruct().Width, Display->GetFramebufferStruct().Height,
		 Display->GetFramebufferStruct().BitsPerPixel,
		 Display->GetFramebufferStruct().RedMaskSize, Display->GetFramebufferStruct().RedMaskShift,
		 Display->GetFramebufferStruct().GreenMaskSize, Display->GetFramebufferStruct().GreenMaskShift,
		 Display->GetFramebufferStruct().BlueMaskSize, Display->GetFramebufferStruct().BlueMaskShift);

	klog("%lld MiB / %lld MiB (%lld MiB reserved)", TO_MiB(KernelAllocator.GetUsedMemory()),
		 TO_MiB(KernelAllocator.GetTotalMemory() - KernelAllocator.GetReservedMemory()),
		 TO_MiB(KernelAllocator.GetReservedMemory()));
#endif

	/**************************************************************************************/

	klog("Reading Kernel Parameters");
	ParseConfig(bInfo.Kernel.CommandLine);

	klog("Loading Kernel Symbols");
	KernelSymbolTable = new SymbolResolver::Symbols((uintptr_t)bInfo.Kernel.FileBase);

	if (!KernelSymbolTable->SymTableExists)
		KernelSymbolTable->AddSymbolInfoFromGRUB(bInfo.Kernel.Symbols.Num,
												 bInfo.Kernel.Symbols.EntSize,
												 bInfo.Kernel.Symbols.Shndx,
												 bInfo.Kernel.Symbols.Sections);

	klog("Initializing Power Manager");
	PowerManager = new Power::Power;

	klog("Initializing PCI Manager");
	PCIManager = new PCI::Manager;

	klog("Initializing Filesystem");
	KernelVFS();

#ifdef DEBUG
	__early_playground();
#endif

	klog("\x1b[1;32m################################\x1b[0m");
	TaskManager = new Tasking::Task(Tasking::IP(main_thread));
	TaskManager->StartScheduler();
	CPU::Halt(true);
}
