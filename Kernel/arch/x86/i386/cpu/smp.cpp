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
#include <acpi.hpp>
#include <irq.hpp>
#include <assert.h>
#include <cpu.hpp>
#include <atomic>

#include "../../../../kernel.h"
#include "apic.hpp"

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

std::atomic_bool CPUEnabled = false;

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static __aligned(PAGE_SIZE) CPUData CPUs[MAX_CPU] = {0};

nsa CPUData *GetCPU(long id) { return &CPUs[id]; }

nsa CPUData *GetCurrentCPU()
{
	if (unlikely(!Interrupts::apic[0]))
		return &CPUs[0]; /* No APIC means we are on the BSP. */

	APIC::APIC *apic = (APIC::APIC *)Interrupts::apic[0];
	int CoreID = 0;
	if (CPUEnabled.load(std::memory_order_acquire) == true)
	{
		if (apic->x2APIC)
			CoreID = int(CPU::x86::rdmsr(CPU::x86::MSR_X2APIC_APICID));
		else
			CoreID = apic->Read(APIC::APIC_ID) >> 24;
	}

	assert((&CPUs[CoreID])->IsActive == true);
	return &CPUs[CoreID];
}

namespace SMP
{
	int CPUCores = 0;

	void Initialize(void *_madt)
	{
		ACPI::MADT *madt = (ACPI::MADT *)_madt;

		int Cores = madt->CPUCores + 1;

		if (Config.Cores > madt->CPUCores + 1)
			klog("More cores requested than available. Using %d cores", madt->CPUCores + 1);
		else if (Config.Cores != 0)
			Cores = Config.Cores;

		CPUCores = Cores;

		fixme("SMP::Initialize() is not implemented!");
	}
}
