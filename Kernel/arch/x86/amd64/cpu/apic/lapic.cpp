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
#include <io.h>

using namespace CPU::x86;

namespace APIC
{
	uint32_t Local::Read(uint32_t Register)
	{
#ifdef DEBUG
		if (Register != APIC_ICRLO && Register != APIC_ICRHI &&
			Register != APIC_ID)
			debug("APIC::Read(%#lx) [x2=%d]",
				  Register, x2APICSupported ? 1 : 0);
#endif
		if (unlikely(x2APICSupported))
			assert(!"x2APIC is not supported");

		CPU::MemBar::Barrier();
		uint32_t ret = *((volatile uint32_t *)(APICBaseAddress + Register));
		CPU::MemBar::Barrier();
		return ret;
	}

	void Local::Write(uint32_t Register, uint32_t Value)
	{
#ifdef DEBUG
		if (Register != APIC_EOI &&
			Register != APIC_TDCR &&
			Register != APIC_TIMER &&
			Register != APIC_TICR &&
			Register != APIC_ICRLO &&
			Register != APIC_ICRHI)
			debug("APIC::Write(%#lx, %#lx) [x2=%d]",
				  Register, Value, x2APICSupported ? 1 : 0);
#endif
		if (unlikely(x2APICSupported))
			assert(!"x2APIC is not supported");

		CPU::MemBar::Barrier();
		*((volatile uint32_t *)((APICBaseAddress) + Register)) = Value;
		CPU::MemBar::Barrier();
	}

	void Local::EOI()
	{
		Memory::SwapPT swap = Memory::SwapPT(KernelPageTable, thisPageTable);
		if (this->x2APICSupported)
			wrmsr(MSR_X2APIC_EOI, 0);
		else
			this->Write(APIC_EOI, 0);
	}

	void Local::WaitForIPI()
	{
		if (this->x2APICSupported)
		{
			ErrorStatusRegister esr{};
			esr.raw = uint32_t(rdmsr(MSR_X2APIC_ESR));
			UNUSED(esr);
			/* FIXME: Not sure if this is required or
				how to implement it. */
		}
		else
		{
			InterruptCommandRegister icr{};
			do
			{
				icr.split.Low = this->Read(APIC_ICRLO);
				CPU::Pause();
			} while (icr.DS != Idle);
		}
	}

	void Local::ICR(InterruptCommandRegister icr)
	{
		if (x2APICSupported)
		{
			assert(icr.MT != LowestPriority);
			assert(icr.MT != DeliveryMode);
			assert(icr.MT != ExtINT);
			wrmsr(MSR_X2APIC_ICR, icr.raw);
			this->WaitForIPI();
		}
		else
		{
			this->Write(APIC_ICRHI, icr.split.High);
			this->Write(APIC_ICRLO, icr.split.Low);
			this->WaitForIPI();
		}
	}

	void Local::SendInitIPI()
	{
		InterruptCommandRegister icr{};

		if (x2APICSupported)
		{
			icr.x2.MT = INIT;
			icr.x2.L = Assert;
			icr.x2.DES = uint8_t(Core);

			wrmsr(MSR_X2APIC_ICR, icr.raw);
			this->WaitForIPI();
		}
		else
		{
			icr.MT = INIT;
			icr.L = Assert;
			icr.DES = uint8_t(Core);

			this->Write(APIC_ICRHI, icr.split.High);
			this->Write(APIC_ICRLO, icr.split.Low);
			this->WaitForIPI();
		}
	}

	void Local::SendStartupIPI(uint64_t StartupAddress)
	{
		InterruptCommandRegister icr{};

		if (x2APICSupported)
		{
			icr.x2.VEC = static_cast<uint8_t>(StartupAddress >> 12);
			icr.x2.MT = Startup;
			icr.x2.L = Assert;
			icr.x2.DES = uint8_t(Core);

			wrmsr(MSR_X2APIC_ICR, icr.raw);
			this->WaitForIPI();
		}
		else
		{
			icr.VEC = static_cast<uint8_t>(StartupAddress >> 12);
			icr.MT = Startup;
			icr.L = Assert;
			icr.DES = uint8_t(Core);

			this->Write(APIC_ICRHI, icr.split.High);
			this->Write(APIC_ICRLO, icr.split.Low);
			this->WaitForIPI();
		}
	}

	Local::Local(int Core) : Core(Core)
	{
		APIC_BASE base = {.raw = rdmsr(MSR_APIC_BASE)};
		uint64_t baseLow = base.ABALow;
		uint64_t baseHigh = base.ABAHigh;
		this->APICBaseAddress = baseLow << 12u | baseHigh << 32u;
		trace("APIC Address: %#lx", this->APICBaseAddress.get());
		Memory::Virtual(KernelPageTable).SingleMap(this->APICBaseAddress, this->APICBaseAddress, Memory::RW | Memory::PCD | Memory::PWT);

		if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
		{
			CPU::x86::AMD::CPUID0x00000001 cpuid;
			if (cpuid.ECX.x2APIC)
			{
				this->x2APICSupported = cpuid.ECX.x2APIC;
				debug("x2APIC is supported");
			}
		}
		else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
		{
			CPU::x86::Intel::CPUID0x00000001 cpuid;
			if (cpuid.ECX.x2APIC)
			{
				this->x2APICSupported = cpuid.ECX.x2APIC;
				debug("x2APIC is supported");
			}
		}

		base.AE = 1;
		wrmsr(MSR_APIC_BASE, base.raw);

		if (this->x2APICSupported)
		{
			base.EXTD = 1;
			wrmsr(MSR_APIC_BASE, base.raw);
		}

		if (!this->x2APICSupported)
		{
			this->Write(APIC_TPR, 0x0);
			this->Write(APIC_DFR, 0xF0000000);
			this->Write(APIC_LDR, this->Read(APIC_ID));
		}
		else
		{
			wrmsr(MSR_X2APIC_TPR, 0x0);
		}

		for (auto &&i : MADTManager->nmi)
		{
			if (i->processor != 0xFF &&
				Core != i->processor)
				break;

			uint32_t nmi = 0x402;
			if (i->flags & 2)
				nmi |= 1 << 13;
			if (i->flags & 8)
				nmi |= 1 << 15;
			if (i->lint == 0)
			{
				if (this->x2APICSupported)
					wrmsr(MSR_X2APIC_LVT_LINT0, nmi);
				else
					this->Write(APIC_LINT0, nmi);
			}
			else if (i->lint == 1)
			{
				if (this->x2APICSupported)
					wrmsr(MSR_X2APIC_LVT_LINT1, nmi);
				else
					this->Write(APIC_LINT1, nmi);
			}
		}

		/* Setup the spurious interrupt vector */
		Spurious svr{};
		if (this->x2APICSupported)
			svr.raw = uint32_t(rdmsr(MSR_X2APIC_SIVR));
		else
			svr.raw = this->Read(APIC_SVR);

		svr.VEC = CPU::x86::IRQ223;
		svr.ASE = 1;
		if (this->x2APICSupported)
			wrmsr(MSR_X2APIC_SIVR, svr.raw);
		else
			this->Write(APIC_SVR, svr.raw);
	}
}
