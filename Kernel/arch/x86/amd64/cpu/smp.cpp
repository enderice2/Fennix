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
	APIC_ID = 0x610,
	TRAMPOLINE_START = 0x2000
};

std::atomic_bool InsideAP = false;
std::atomic_bool CPUEnabled = false;

static __aligned(PAGE_SIZE) CPUData CPUs[MAX_CPU] = {};
__aligned(16) int APICIDToCore[256] = {};

hot CPUData *GetCPU(int id) { return &CPUs[id]; }

hot CPUData *GetCurrentCPU()
{
	if (unlikely(InsideAP.load(std::memory_order_relaxed)))
		return &CPUs[(int)*reinterpret_cast<int *>(CORE)];

	int id = APICIDToCore[CPU::CurrentID()];
	assert(CPUs[id].IsActive == true || id == 0);
	return &CPUs[id];
}

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
	int apic_id = (int)*reinterpret_cast<int *>(APIC_ID);
	CPU::Initialize(id);
	CPU::InitializeInterrupts(id);
	if (APICManager)
		APICManager->SetupCore(id, (uint8_t)apic_id);
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

		for (int i = 0; i < totalCores; i++)
		{
			auto lapic = madt->lapic[i];
			APICIDToCore[lapic->APICId] = i;
			debug("%d: ACPI Processor ID=%d, LAPIC ID=%d, Flags=%#x",
				  i, lapic->ACPIProcessorId, lapic->APICId, lapic->Flags);
		}

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

			uint8_t apicID = APICManager->GetID();
			debug("init core %d: ACPI Processor=%d LAPIC=%d APIC_ID=%d", i, lapic->ACPIProcessorId, lapic->APICId, apicID);
			if (apicID != lapic->APICId)
			{
				VPOKE(int, CORE) = i;
				VPOKE(int, APIC_ID) = int(lapic->APICId);
				if (!apic->x2APIC)
				{
					APIC::InterruptCommandRegister icr{};
					icr.MT = APIC::INIT;
					icr.DES = lapic->APICId;
					apic->ICR(icr);
				}
				thisClock->Sleep(10ms);

				debug("Init IPI");
				{
					APIC::InterruptCommandRegister icr{};

					if (apic->x2APIC)
					{
						icr.x2.MT = APIC::INIT;
						icr.x2.L = APIC::Assert;
						icr.x2.DES = lapic->APICId;

						CPU::x86::wrmsr(CPU::x86::MSR_X2APIC_ICR, icr.raw);
						apic->WaitForIPI();
					}
					else
					{
						icr.MT = APIC::INIT;
						icr.L = APIC::Assert;
						icr.DES = lapic->APICId;

						apic->Write(APIC::APIC_ICRHI, icr.split.High);
						apic->Write(APIC::APIC_ICRLO, icr.split.Low);
						apic->WaitForIPI();
					}
				}

				thisClock->Sleep(50ms);

				debug("Startup IPI");
				{
					APIC::InterruptCommandRegister icr{};

					if (apic->x2APIC)
					{
						icr.x2.VEC = static_cast<uint8_t>(TRAMPOLINE_START >> 12);
						icr.x2.MT = APIC::Startup;
						icr.x2.L = APIC::Assert;
						icr.x2.DES = lapic->APICId;

						CPU::x86::wrmsr(CPU::x86::MSR_X2APIC_ICR, icr.raw);
						apic->WaitForIPI();
					}
					else
					{
						icr.VEC = static_cast<uint8_t>(TRAMPOLINE_START >> 12);
						icr.MT = APIC::Startup;
						icr.L = APIC::Assert;
						icr.DES = lapic->APICId;

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
