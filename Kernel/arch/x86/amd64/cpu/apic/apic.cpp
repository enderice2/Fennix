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

#include <cpu/apic.hpp>

#include <memory.hpp>
#include <entry.hpp>
#include <uart.hpp>
#include <lock.hpp>
#include <cpu.hpp>
#include <smp.hpp>

#include "../../../../kernel.h"

NewLock(APICLock);

using namespace CPU::x64;
using namespace CPU::x86;

namespace APIC
{
	// headache
	// https://www.amd.com/system/files/TechDocs/24593.pdf
	// https://www.naic.edu/~phil/software/intel/318148.pdf

	Local *APIC::SetupCore(int ID)
	{
		assert(Locals[ID] == nullptr);

		Local *lapic = new Local(ID);
		Locals[ID] = lapic;
		return lapic;
	}

	Ofast hot int APIC::GetID()
	{
		Local *lapic = Locals[0];
		if (unlikely(lapic == nullptr))
			return 0;
		if (lapic->x2APIC)
			return int(CPU::x86::rdmsr(CPU::x86::MSR_X2APIC_APICID));
		else
			return lapic->Read(APIC_ID) >> 24;
	}

	void APIC::Mask(uint32_t Line) { IOAPIC.Mask(Line); }
	void APIC::Unmask(uint32_t Line) { IOAPIC.Unmask(Line); }
	uint32_t APIC::TranslateToLine(uint32_t Vector) { return Vector - CPU::x86::IRQ0; }
	void APIC::EOI(uint32_t Vector) { Locals[GetID()]->EOI(); }
	void APIC::Route(uint32_t Vector, uint32_t Core) { IOAPIC.RedirectIRQ(Core, TranslateToLine(Vector), 1); }

	APIC::APIC()
	{
		for (int i = 0; i < MAX_CPU; i++)
			Locals.push_back(nullptr);

		static int once = 0;
		if (!once++)
		{
			// Disable PIT
			outb(0x43, 0x28);
			outb(0x40, 0x0);

			// Disable PIC
			outb(0x21, 0xFF);
			outb(0xA1, 0xFF);
		}
	}

	APIC::~APIC()
	{
		for (std::size_t i = 0; i < Locals.size(); i++)
			delete Locals[i];
	}
}
