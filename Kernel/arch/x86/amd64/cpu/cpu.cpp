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

#include <cpu.hpp>

#include <memory.hpp>
#include <cpu/apic.hpp>
#include <convert.h>
#include <debug.h>
#include <smp.hpp>
#include <entry.hpp>

using namespace CPU::x64;

#include "gdt.hpp"
#include "idt.hpp"
#include "tsc.hpp"
#include "pit.hpp"
#include "../firmware/acpi/hpet.hpp"

Time::ClockSource *GlobalClock = nullptr;
Time::TimerDevice *GlobalTimer = nullptr;

extern Interrupt::Manager irq;

namespace CPU
{
	struct SupportedFeat
	{
		bool PGE = false;
		bool SSE = false;
		bool UMIP = false;
		bool SMEP = false;
		bool SMAP = false;
	};

	SupportedFeat GetCPUFeat()
	{
		SupportedFeat feat = {};

		if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
		{
			CPU::x86::AMD::CPUID0x00000001 cpuid1;
			CPU::x86::AMD::CPUID0x00000007_ECX_0 cpuid7;

			feat.PGE = cpuid1.EDX.PGE;
			feat.SSE = cpuid1.EDX.SSE;
			feat.SMEP = cpuid7.EBX.SMEP;
			feat.SMAP = cpuid7.EBX.SMAP;
			feat.UMIP = cpuid7.ECX.UMIP;
		}
		else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
		{
			CPU::x86::Intel::CPUID0x00000001 cpuid1;
			CPU::x86::Intel::CPUID0x00000007_0 cpuid7_0;

			feat.PGE = cpuid1.EDX.PGE;
			feat.SSE = cpuid1.EDX.SSE;
			feat.SMEP = cpuid7_0.EBX.SMEP;
			feat.SMAP = cpuid7_0.EBX.SMAP;
			feat.UMIP = cpuid7_0.ECX.UMIP;
		}

		return feat;
	}

	void Initialize(int CoreID)
	{
		CPUData *core = GetCPU(CoreID);
		core->ID = CoreID;
		core->IsActive = true;
		Log::MemoryInit(CoreID);
		core->Stack = StackManager.Allocate(STACK_SIZE) + STACK_SIZE;
		debug("cpu stack: %#lx-%#lx", core->Stack - STACK_SIZE, core->Stack);

		GlobalDescriptorTable::Setup(CoreID);
		InterruptDescriptorTable::Setup(CoreID);
		CPU::x86::wrmsr(CPU::x86::MSR_GS_BASE, (uint64_t)core);
		CPU::x86::wrmsr(CPU::x86::MSR_SHADOW_GS_BASE, (uint64_t)core);
		InitializeSystemCalls();

		CR0 cr0 = readcr0();
		CR4 cr4 = readcr4();

		SupportedFeat feat = GetCPUFeat();
		/* Not sure if my code is not working properly or something else is the issue. */
		if ((strcmp(Hypervisor(), x86_CPUID_VENDOR_VIRTUALBOX) != 0) && feat.SSE)
		{
			debug("Enabling SSE support");
			cr0.EM = false;
			cr0.MP = true;
			cr4.OSFXSR = true;
			cr4.OSXMMEXCPT = true;

			/* I have no idea what this does */
			core->Data.FPU.MXCSR.raw = 0b0001111110000000;
			core->Data.FPU.MXCSR_MASK = 0b1111111110111111;
			core->Data.FPU.FCW.raw = 0b0000001100111111;
			CPU::x86::fxrstor(&core->Data.FPU);
		}

		/* More info in AMD64 Architecture Programmer's Manual
			Volume 2: 3.1.1 CR0 Register */

		/* Not Write-Through
			This is ignored on recent processors.
		*/
		cr0.NW = false;

		/* Cache Disable
			Wether the CPU should cache memory or not.
			If this bit is enabled, PWT and PCD are ignored.
		*/
		cr0.CD = false;

		/* Write Protect
			When set, the supervisor can't write to read-only pages.
		*/
		cr0.WP = true;

		/* Alignment check
			The CPU checks the alignment of memory operands
			and generates #AC if the alignment is incorrect.

			The condition for an alignment check is:
			- The AM flag in CR0 is set.
			- The AC flag in the RFLAGS register is set.
			- CPL is 3.
		*/
		cr0.AM = true;

		debug("CPU Prevention Features:%s%s%s",
			  feat.SMEP ? " SMEP" : "",
			  feat.SMAP ? " SMAP" : "",
			  feat.UMIP ? " UMIP" : "");

		/* User-Mode Instruction Prevention
			This prevents user-mode code from executing these instructions:
				SGDT, SIDT, SLDT, SMSW, STR
			If any of these instructions are executed with CPL > 0, a #GP is generated.
		*/
		// cr4.UMIP = feat.UMIP;

		/* Supervisor Mode Execution Prevention
			This prevents user-mode code from executing code in the supervisor mode.
		*/
		cr4.SMEP = feat.SMEP;

		/* Supervisor Mode Access Prevention
			This prevents supervisor-mode code from accessing user-mode pages.
		*/
		cr4.SMAP = feat.SMAP;

		/* Page Global Enable
			Enables the use of the PGE bit in the page table entries.
		*/
		cr4.PGE = feat.PGE;

		debug("updating CR0");
		writecr0(cr0);
		debug("updating CR4");
		writecr4(cr4);

		debug("enabling PAT support");
		CPU::x86::wrmsr(CPU::x86::MSR_CR_PAT, 0x6 | (0x0 << 8) | (0x1 << 16));
	}

	void InitializeInterrupts(int CoreID)
	{
		bool APICSupported = false;
		if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
		{
			CPU::x86::AMD::CPUID0x00000001 cpuid;
			APICSupported = cpuid.EDX.APIC;
		}
		else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
		{
			CPU::x86::Intel::CPUID0x00000001 cpuid;
			APICSupported = cpuid.EDX.APIC;
		}

		if (MADTManager != nullptr && APICSupported)
		{
			if (APICManager == nullptr)
			{
				APICManager = new APIC::APIC;
				irq.Initialize(*APICManager);
			}
			APICManager->SetupCore(CoreID);
		}
		else
		{
			static int once = 0;
			if (!once++)
			{
				// PIC
			}
		}
		asmv("sti");
	}

	void InitializeTimeSources(int CoreID)
	{
		/*
			For ClockSource:
			1 - Invariant TSC (local)
			2 - kvmclock
			3 - HPET          (global)
			4 - ACPI PM Timer (global)
			5 - PIT           (global)

			For TimerSource:
			1 - TSC-Deadline     (local)
			2 - kvmclock
			3 - Local APIC Timer (local)
			4 - HPET             (global)
			5 - PIT              (global)
		*/

		static Time::ClockSource *baseClock = nullptr;
		static Time::TimerDevice *baseTimer = nullptr;

		if (CoreID == 0)
		{
			/* prefer HPET over everything calibrated */
			Platform::HighPrecisionEventTimer *hpet = nullptr;
			if (ACPIManager->HPET != nullptr)
			{
				hpet = new Platform::HighPrecisionEventTimer(ACPIManager->HPET);
				baseClock = hpet;
				baseTimer = hpet;
			}

			// TODO: ACPI PM Timer fallback

			ProgrammableIntervalTimer::PIT *pit = nullptr;
			if (baseClock == nullptr)
			{
				pit = new ProgrammableIntervalTimer::PIT;
				baseClock = pit;
			}

			GlobalClock = baseClock;
			GlobalTimer = baseTimer;

			thisClock = baseClock;
			thisTimer = baseTimer;
		}
		else
		{
			thisClock = baseClock;
			thisTimer = baseTimer;
		}

		/* TSC needs calibration */
		// TimeStampCounter::TSC *tsc = new TimeStampCounter::TSC;
		// if (tsc->Invariant())
		// 	tsc->Calibrate();
		// else
		// 	delete tsc, tsc = nullptr;

		// if (tsc)
		// 	thisClock = tsc;

		/* APIC Timer also needs calibration */
		APIC::Timer *timer = nullptr;
		if (APICManager != nullptr)
			timer = new APIC::Timer(APICManager->Locals[CoreID]);

		if (timer)
			thisTimer = timer;
	}

	Ofast hot uint32_t CurrentID()
	{
		if (unlikely(APICManager == nullptr))
			return 0;
		return APICManager->GetID();
	}

	Ofast bool Interrupts(InterruptsType Type)
	{
		switch (Type)
		{
		case Check:
		{
			uintptr_t Flags;
			asmv("pushfq");
			asmv("popq %0" : "=r"(Flags));
			return Flags & (1 << 9);
		}
		case Enable:
		{
			asmv("sti");
			return true;
		}
		case Disable:
		{
			asmv("cli");
			return true;
		}
		default:
			assert(!"Unknown InterruptsType");
		}
	}

	fnx::void_t PageTable(fnx::void_t PT)
	{
		void *ret;
		asmv("movq %%cr3, %0" : "=r"(ret));

		if (PT.get())
			asmv("movq %0, %%cr3" : : "r"(PT.get()) : "memory");
		return ret;
	}

	uint64_t Counter()
	{
		// TODO: Get the counter from the x2APIC or any other timer that is available. (TSC is not available on all CPUs)
		uint64_t Counter;
		uint32_t eax, edx;
		asmv("rdtsc" : "=a"(eax),
			 "=d"(edx));
		Counter = ((uint64_t)eax) | (((uint64_t)edx) << 32);
		return Counter;
	}
}
