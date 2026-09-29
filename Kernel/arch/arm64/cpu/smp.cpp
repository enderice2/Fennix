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

#include <irq.hpp>
#include <memory.hpp>
#include <cpu.hpp>

#include "../../../../kernel.h"

volatile bool CPUEnabled = false;

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static __aligned(0x1000) CPUData CPUs[MAX_CPU] = {0};

CPUData *GetCPU(uint64_t id) { return &CPUs[id]; }

CPUData *GetCurrentCPU()
{
	uint64_t ret = 0;

	if (!CPUs[ret].IsActive)
	{
		error("CPU %d is not active!", ret);
		return &CPUs[0];
	}

	return &CPUs[ret];
}

namespace SMP
{
	int CPUCores = 0;

	void Initialize(void *madt)
	{
		fixme("SMP::Initialize() is not implemented!");
	}
}
