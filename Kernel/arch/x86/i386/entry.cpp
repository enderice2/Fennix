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

#include <types.h>

#include <boot/binfo.h>
#include <memory.hpp>
#include <convert.h>
#include <log.hpp>
#include <cpu.hpp>
#include <efi.h>
#include <io.h>

extern struct BootInfo bInfo;
extern bool DebuggerIsAttached;

void Test_stl();
void TestMemoryAllocation();
int main();
extern "C" int _init();
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

extern "C" __no_stack_protector nif cold void KernelEntry(BootInfo *Info)
{
	Log::EarlyInit();
	memcpy(&bInfo, Info, sizeof(BootInfo));
	_init();

	if (strcmp(CPU::Hypervisor(), x86_CPUID_VENDOR_TCG) == 0)
		DebuggerIsAttached = true;

	if (bInfo.EFI.Info.IH || bInfo.EFI.Info.ST)
		efi_main((EFI_HANDLE)bInfo.EFI.ImageHandle, (EFI_SYSTEM_TABLE *)bInfo.EFI.SystemTable);

#if defined(__amd64__) || defined(__i386__)
	if (!bInfo.SMBIOSPtr)
	{
		trace("SMBIOS was not provided by the bootloader. Trying to find it manually.");
		for (uintptr_t i = 0xF0000; i < 0x100000; i += 16)
		{
			if (memcmp((void *)i, "_SM_", 4) == 0 ||
				memcmp((void *)i, "_SM3_", 5) == 0)
			{
				bInfo.SMBIOSPtr = (void *)i;
				trace("Found SMBIOS at %#lx", i);
			}
		}
	}

	if (!bInfo.RSDP)
	{
		trace("RSDP was not provided by the bootloader. Trying to find it manually.");
		/* FIXME: Not always shifting by 4 will work. */
		uintptr_t EBDABase = (uintptr_t)mminw((void *)0x40E) << 4;

		for (uintptr_t ptr = EBDABase;
			 ptr < 0x100000; /* 1MB */
			 ptr += 16)
		{
			if (unlikely(ptr == EBDABase + 0x400))
			{
				trace("EBDA is full. Trying to find RSDP in the BIOS area.");
				break;
			}

			BootInfo::RSDPInfo *rsdp = (BootInfo::RSDPInfo *)ptr;
			if (memcmp(rsdp->Signature, "RSD PTR ", 8) == 0)
			{
				bInfo.RSDP = (BootInfo::RSDPInfo *)rsdp;
				trace("Found RSDP at %#lx", rsdp);
			}
		}

		for (uintptr_t ptr = 0xE0000;
			 ptr < 0x100000; /* 1MB */
			 ptr += 16)
		{
			BootInfo::RSDPInfo *rsdp = (BootInfo::RSDPInfo *)ptr;
			if (memcmp(rsdp->Signature, "RSD PTR ", 8) == 0)
			{
				bInfo.RSDP = (BootInfo::RSDPInfo *)rsdp;
				trace("Found RSDP at %#lx", rsdp);
			}
		}
	}
#endif

	Memory::Initialize();

	void *KernelStackAddress = KernelAllocator.RequestPages(TO_PAGES(STACK_SIZE));
	// void *KernelStackAddress = StackManager.Allocate(STACK_SIZE); /* FIXME: This breaks stl tests, how? */
	uintptr_t KernelStack = (uintptr_t)KernelStackAddress + STACK_SIZE - 0x10;
	debug("Kernel stack: %#lx-%#lx", KernelStackAddress, KernelStack);

	asmv("mov %0, %%esp" : : "r"(KernelStack) : "memory");
	asmv("mov $0, %ebp");

#ifdef DEBUG
	/* I had to do this because KernelAllocator
	 * is a global constructor but we need
	 * memory management to be initialized first.
	 */
	TestMemoryAllocation();
	Test_stl();
#endif // DEBUG
	main();
}
