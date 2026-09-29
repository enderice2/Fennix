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
#include <cpu/apic.hpp>
#include <memory.hpp>
#include <convert.h>
#include <acpi.hpp>
#include <time.hpp>
#include <log.hpp>
#include <cpu.hpp>
#include <smp.hpp>
#include <efi.h>
#include <io.h>

extern struct BootInfo bInfo;
extern bool DebuggerIsAttached;

int main();
void Test_stl();
void TestMemoryAllocation();
void *FindSMBIOS();
BootInfo::RSDPInfo *FindRSDP();
extern "C" int _init();
extern "C" int _fini();
extern "C" void __cxa_finalize(void *);

Platform::ACPI *ACPIManager = nullptr;
Platform::MADT *MADTManager = nullptr;
Platform::DSDT *DSDTManager = nullptr;
APIC::APIC *APICManager = nullptr;

extern "C" __no_stack_protector nif cold void KernelEntry(BootInfo *Info)
{
	Log::EarlyInit();
	memcpy(&bInfo, Info, sizeof(BootInfo));

	if (strcmp(CPU::Hypervisor(), x86_CPUID_VENDOR_TCG) == 0)
		DebuggerIsAttached = true;

	if (bInfo.SMBIOSPtr == nullptr)
	{
		warn("SMBIOS was not provided by the bootloader. Trying to find it manually.");
		bInfo.SMBIOSPtr = FindSMBIOS();
	}

	if (bInfo.RSDP == nullptr)
	{
		warn("RSDP was not provided by the bootloader. Trying to find it manually.");
		bInfo.RSDP = FindRSDP();
	}

	_init();

	Memory::Initialize();

	klog("Initializing CPU");
	CPU::Initialize();

	/* "Stack" is initialized in CPU::Initialize */
	asmv("mov %0, %%rsp" : : "r"(GetCPU(0)->Stack) : "memory");
	asmv("mov $0, %rbp");

	klog("Initializing ACPI");
	ACPIManager = new Platform::ACPI;
	if (ACPIManager->IsPresent())
	{
		MADTManager = new Platform::MADT(ACPIManager->MADT);
		DSDTManager = new Platform::DSDT(ACPIManager);
	}
	else
		delete ACPIManager, ACPIManager = nullptr;

	klog("Initializing Interrupts");
	CPU::InitializeInterrupts();
	klog("Initializing Timer & Clock Sources");
	CPU::InitializeTimeSources();
	klog("Initializing SMP");
	SMP::Initialize();

#ifdef DEBUG
	/* I had to do this because KernelAllocator
	 * is a global constructor but we need
	 * memory management to be initialized first.
	 */
	TestMemoryAllocation();
	Test_stl();
#endif // DEBUG

	main();
	__unreachable;
}

cold void KernelExit()
{
	_fini();
	__cxa_finalize(nullptr);
}
