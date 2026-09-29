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

#include <smp.hpp>

#include <memory.hpp>
#include <entry.hpp>
#include <acpi.hpp>
#include <irq.hpp>
#include <assert.h>
#include <cpu.hpp>
#include <atomic>

#include "../../../../kernel.h"

extern "C" uint64_t _trampoline_start, _trampoline_end;

using namespace std::chrono_literals;

/* https://wiki.osdev.org/Memory_Map_(x86) */
enum SMPTrampolineAddress
{
	PAGE_TABLE = 0x500,
	START_ADDR = 0x520,
	STACK = 0x570,
	GDT = 0x580,
	IDT = 0x590,
	CORE = 0x600,
	TRAMPOLINE_START = 0x2000
};

static __aligned(PAGE_SIZE) CPUData CPUs[MAX_CPU] = {};

hot CPUData *GetCPU(int id) { return &CPUs[id]; }

std::atomic_bool InsideAP = false;
hot CPUData *GetCurrentCPU()
{
	int id;
	if (unlikely(InsideAP.load(std::memory_order_relaxed)))
		id = (int)*reinterpret_cast<int *>(CORE);
	else
		id = CPU::CurrentID();
	assert(!(CPUs[id].IsActive == false && id != 0));
	return &CPUs[id];
}

std::atomic_bool CPUEnabled = false;

extern "C" __noreturn void APHalt()
{
	klog("CPU %d is online", thisCPU->ID);
	CPUEnabled.store(true, std::memory_order_release);
halt:
	asmv("hlt");
	goto halt;
	__unreachable;
}

extern "C" __noreturn void StartCPU()
{
	asmv("cli");
	InsideAP.store(true);

	int id = (int)*reinterpret_cast<int *>(CORE);
	CPU::Initialize(id);
	CPU::InitializeInterrupts(id);
	CPU::InitializeTimeSources(id);

	InsideAP.store(false);
	asmv("mov %0, %%rsp\n"
		 "xor %%rbp, %%rbp\n"
		 "jmp *%1\n" : : "r"(GetCPU(id)->Stack),
		 "r"(APHalt) : "memory");
	__unreachable;
}

namespace SMP
{
	int CPUCores = 0;

	void Initialize()
	{
		if (MADTManager == nullptr)
			return;

		Platform::MADT *madt = MADTManager;

		if (madt->lapic.size() < 1)
			return;

		int totalCores = madt->CPUCores + 1;

		if (Config.Cores > madt->CPUCores + 1)
			klog("More cores requested than available. Using %d cores", madt->CPUCores + 1);
		else if (Config.Cores != 0)
			totalCores = Config.Cores;

		CPUCores = totalCores;

		uint64_t trampolineLength = (uintptr_t)&_trampoline_end - (uintptr_t)&_trampoline_start;
		Memory::Virtual().SingleMap(0x0, 0x0, Memory::PTFlag::RW);
		/* We reserved the TRAMPOLINE_START address inside Physical class. */
		Memory::Virtual().Map(TRAMPOLINE_START, TRAMPOLINE_START, trampolineLength, Memory::PTFlag::RW);
		memcpy((void *)TRAMPOLINE_START, &_trampoline_start, trampolineLength);
		debug("Trampoline address: %#lx-%#lx", TRAMPOLINE_START, TRAMPOLINE_START + trampolineLength);

		void *CPUTmpStack = KernelAllocator.RequestPages(TO_PAGES(STACK_SIZE));
		asmv("sgdt [0x580]");
		asmv("sidt [0x590]");
		VPOKE(uintptr_t, STACK) = (uintptr_t)CPUTmpStack + STACK_SIZE;
		VPOKE(uintptr_t, PAGE_TABLE) = (uintptr_t)KernelPageTable;
		VPOKE(uintptr_t, START_ADDR) = (uintptr_t)&StartCPU;

		for (int i = 1; i < totalCores; i++)
		{
			auto lapic = madt->lapic[i];
			auto apic = APICManager->Locals[0];

			debug("Initializing CPU %d", lapic->APICId);
			uint8_t APIC_ID = 0;
			if (apic->x2APIC)
				APIC_ID = uint8_t(CPU::x86::rdmsr(CPU::x86::MSR_X2APIC_APICID));
			else
				APIC_ID = uint8_t(apic->Read(APIC::APIC_ID) >> 24);

			if (APIC_ID != lapic->APICId)
			{
				VPOKE(int, CORE) = i;
				if (!apic->x2APIC)
				{
					APIC::InterruptCommandRegister icr{};
					icr.MT = APIC::INIT;
					icr.DES = lapic->APICId;
					apic->ICR(icr);
				}

				/* Init IPI */
				{
					APIC::InterruptCommandRegister icr{};

					if (apic->x2APIC)
					{
						icr.x2.MT = APIC::INIT;
						icr.x2.L = APIC::Assert;
						icr.x2.DES = uint8_t(i);

						CPU::x86::wrmsr(CPU::x86::MSR_X2APIC_ICR, icr.raw);
						apic->WaitForIPI();
					}
					else
					{
						icr.MT = APIC::INIT;
						icr.L = APIC::Assert;
						icr.DES = uint8_t(i);

						apic->Write(APIC::APIC_ICRHI, icr.split.High);
						apic->Write(APIC::APIC_ICRLO, icr.split.Low);
						apic->WaitForIPI();
					}
				}

				thisClock->Sleep(50ms);

				/* Startup IPI */
				{
					APIC::InterruptCommandRegister icr{};

					if (apic->x2APIC)
					{
						icr.x2.VEC = static_cast<uint8_t>(TRAMPOLINE_START >> 12);
						icr.x2.MT = APIC::Startup;
						icr.x2.L = APIC::Assert;
						icr.x2.DES = uint8_t(i);

						CPU::x86::wrmsr(CPU::x86::MSR_X2APIC_ICR, icr.raw);
						apic->WaitForIPI();
					}
					else
					{
						icr.VEC = static_cast<uint8_t>(TRAMPOLINE_START >> 12);
						icr.MT = APIC::Startup;
						icr.L = APIC::Assert;
						icr.DES = uint8_t(i);

						apic->Write(APIC::APIC_ICRHI, icr.split.High);
						apic->Write(APIC::APIC_ICRLO, icr.split.Low);
						apic->WaitForIPI();
					}
				}

				// debug("Waiting for CPU %d to load...", lapic->APICId);

				// uint64_t timeout = GlobalClock->Now() + Time::FromSeconds(2);
				while (CPUEnabled.exchange(false, std::memory_order_acq_rel) == false)
				{
					// if (GlobalClock->Now() > timeout)
					// {
					// 	error("CPU %d failed to load!", lapic->APICId);
					// 	break;
					// }
					CPU::Pause();
				}
			}
			else
				klog("CPU %d is the BSP", lapic->APICId);
		}

		KernelAllocator.FreePages(CPUTmpStack, TO_PAGES(STACK_SIZE));
		/* We are going to unmap the page after we are done with it. */
		Memory::Virtual().Unmap(0x0);
		CPUEnabled.store(true, std::memory_order_release);
	}
}
