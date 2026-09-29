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

#include <entry.hpp>

namespace APIC
{
	uint32_t IO::Read(uint64_t Base, uint32_t Register)
	{
		debug("IO::Read(%#lx, %#lx)", Base, Register);
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base))) = Register;
		CPU::MemBar::Barrier();
		uint32_t ret = *((volatile uint32_t *)(((uintptr_t)Base + 16)));
		CPU::MemBar::Barrier();
		return ret;
	}

	void IO::Write(uint64_t Base, uint32_t Register, uint32_t Value)
	{
		debug("IO::Write(%#lx, %#lx, %#lx)", Base, Register, Value);
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base))) = Register;
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base + 16))) = Value;
		CPU::MemBar::Barrier();
	}

	uint32_t IO::GetMaxRedirect(uint32_t APICID)
	{
		Platform::MADT::MADTIOApic *ioapic = MADTManager->ioapic[APICID];
		uint32_t TableAddress = (this->Read(ioapic->Address, GetIOAPICVersion));
		IOAPICVersion ver = {.raw = TableAddress};
		return ver.MLE + 1;
	}

	void IO::RawRedirectIRQ(uint8_t Vector, uint32_t GSI, uint16_t Flags, uint8_t CPU, int Status)
	{
		int64_t IOAPICTarget = -1;
		for (size_t i = 0; i < MADTManager->ioapic.size(); i++)
		{
			if (MADTManager->ioapic[i]->GSIBase <= GSI)
			{
				if (MADTManager->ioapic[i]->GSIBase + this->GetMaxRedirect(uint32_t(i)) > GSI)
				{
					IOAPICTarget = i;
					break;
				}
			}
		}

		if (IOAPICTarget == -1)
		{
			debug("No ISO table found for I/O APIC");
			return;
		}

		IOAPICRedirectEntry Entry{};
		Entry.VEC = Vector;
		Entry.DES = CPU;

		if (Flags & ActiveHighLow)
			Entry.IPP = 1;

		if (Flags & EdgeLevel)
			Entry.TGM = 1;

		if (!Status)
			Entry.M = 1;

		uint32_t IORegister = (GSI - MADTManager->ioapic[IOAPICTarget]->GSIBase) * 2 + 16;
		this->Write(MADTManager->ioapic[IOAPICTarget]->Address, IORegister, Entry.split.Low);
		this->Write(MADTManager->ioapic[IOAPICTarget]->Address, IORegister + 1, Entry.split.High);
	}

	void IO::RedirectIRQ(uint8_t CPU, uint8_t IRQ, int Status)
	{
		for (auto &i : MADTManager->iso)
		{
			if (i->IRQSource != IRQ)
				continue;

			debug("[ISO %d] Mapping to source IRQ%#d GSI:%#lx on CPU %d", i, i->IRQSource, i->GSI, CPU);

			this->RawRedirectIRQ(i->IRQSource + 16,
								 i->GSI,
								 i->Flags,
								 CPU, Status);
			return;
		}

		debug("Mapping IRQ%d on CPU %d", IRQ, CPU);
		this->RawRedirectIRQ(IRQ + 16, IRQ, 0, CPU, Status);
	}

	void IO::RedirectIRQs(uint8_t CPU)
	{
		for (uint8_t i = 0; i < 16; i++)
			this->RedirectIRQ(CPU, i, 1);
	}

	void IO::Mask(uint32_t gsi)
	{
		/* FIXME: untested code! */
		this->RawRedirectIRQ(gsi + 16, gsi, 0, 0, 0);
	}

	void IO::Unmask(uint32_t gsi)
	{
		/* FIXME: untested code! */
		this->RawRedirectIRQ(gsi + 16, gsi, 0, 0, 1);
	}

	IO::IO() { assert(MADTManager != nullptr); }
}
