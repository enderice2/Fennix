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
#include <convert.h>
#include <debug.h>
#include <smp.hpp>

using namespace CPU::x64;

#include "gdt.hpp"
#include "idt.hpp"

namespace CPU
{
	const char *Vendor()
	{
		static char Vendor[13] = {0};
		if (Vendor[0] != 0)
			return Vendor;

		uint32_t eax, ebx, ecx, edx;
		cpuid(0x0, &eax, &ebx, &ecx, &edx);
		memcpy(Vendor + 0, &ebx, 4);
		memcpy(Vendor + 4, &edx, 4);
		memcpy(Vendor + 8, &ecx, 4);
		return Vendor;
	}

	const char *Name()
	{
		static char Name[49] = {0};
		if (Name[0] != 0)
			return Name;

		uint32_t eax, ebx, ecx, edx;
		cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
		memcpy(Name + 0, &eax, 4);
		memcpy(Name + 4, &ebx, 4);
		memcpy(Name + 8, &ecx, 4);
		memcpy(Name + 12, &edx, 4);
		cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
		memcpy(Name + 16, &eax, 4);
		memcpy(Name + 20, &ebx, 4);
		memcpy(Name + 24, &ecx, 4);
		memcpy(Name + 28, &edx, 4);
		cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
		memcpy(Name + 32, &eax, 4);
		memcpy(Name + 36, &ebx, 4);
		memcpy(Name + 40, &ecx, 4);
		memcpy(Name + 44, &edx, 4);
		return Name;
	}

	const char *Hypervisor()
	{
		static char Hypervisor[13] = {0};
		if (Hypervisor[0] != 0)
			return Hypervisor;

		uint32_t eax, ebx, ecx, edx;
		cpuid(0x1, &eax, &ebx, &ecx, &edx);
		if (!(ecx & (1 << 31))) /* Intel & AMD are the same */
		{
			Hypervisor[0] = 'N';
			Hypervisor[1] = 'o';
			Hypervisor[2] = 'n';
			Hypervisor[3] = 'e';
			return Hypervisor;
		}

		cpuid(0x40000000, &eax, &ebx, &ecx, &edx);
		memcpy(Hypervisor + 0, &ebx, 4);
		memcpy(Hypervisor + 4, &ecx, 4);
		memcpy(Hypervisor + 8, &edx, 4);
		return Hypervisor;
	}
}
