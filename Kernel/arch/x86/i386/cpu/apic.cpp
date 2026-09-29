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

#include "apic.hpp"

#include <memory.hpp>
#include <uart.hpp>
#include <lock.hpp>
#include <acpi.hpp>
#include <cpu.hpp>
#include <smp.hpp>
#include <io.h>

#include "../../../../kernel.h"

NewLock(APICLock);

using namespace CPU::x32;
using namespace CPU::x86;

/*
In constructor 'APIC::APIC::APIC(int)':
warning: left shift count >= width of type
|         APICBaseAddress = BaseStruct.ApicBaseLo << 12u | BaseStruct.ApicBaseHi << 32u;
|                                                          ~~~~~~~~~~~~~~~~~~~~~~^~~~~~
*/
#pragma GCC diagnostic ignored "-Wshift-count-overflow"

namespace APIC
{
	// headache
	// https://www.amd.com/system/files/TechDocs/24593.pdf
	// https://www.naic.edu/~phil/software/intel/318148.pdf

	uint32_t APIC::Read(uint32_t Register)
	{
#ifdef DEBUG
		if (Register != APIC_ICRLO &&
			Register != APIC_ICRHI &&
			Register != APIC_ID)
			debug("APIC::Read(%#lx) [x2=%d]", Register, x2APICSupported ? 1 : 0);
#endif
		if (x2APICSupported)
		{
			if (Register != APIC_ICRHI)
				return static_cast<uint32_t>(rdmsr((Register >> 4) + 0x800));
			else
				return static_cast<uint32_t>(rdmsr(0x30 + 0x800));
		}
		else
		{
			CPU::MemBar::Barrier();
			uint32_t ret = *((volatile uint32_t *)((uintptr_t)APICBaseAddress + Register));
			CPU::MemBar::Barrier();
			return ret;
		}
	}

	void APIC::Write(uint32_t Register, uint32_t Value)
	{
#ifdef DEBUG
		if (Register != APIC_EOI &&
			Register != APIC_TDCR &&
			Register != APIC_TIMER &&
			Register != APIC_TICR &&
			Register != APIC_ICRLO &&
			Register != APIC_ICRHI)
			debug("APIC::Write(%#lx, %#lx) [x2=%d]", Register, Value, x2APICSupported ? 1 : 0);
#endif
		if (x2APICSupported)
		{
			if (Register != APIC_ICRHI)
				wrmsr((Register >> 4) + 0x800, Value);
			else
				wrmsr(MSR_X2APIC_ICR, Value);
		}
		else
		{
			CPU::MemBar::Barrier();
			*((volatile uint32_t *)(((uintptr_t)APICBaseAddress) + Register)) = Value;
			CPU::MemBar::Barrier();
		}
	}

	void APIC::IOWrite(uint64_t Base, uint32_t Register, uint32_t Value)
	{
		debug("APIC::IOWrite(%#lx, %#lx, %#lx)", Base, Register, Value);
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base))) = Register;
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base + 16))) = Value;
		CPU::MemBar::Barrier();
	}

	uint32_t APIC::IORead(uint64_t Base, uint32_t Register)
	{
		debug("APIC::IORead(%#lx, %#lx)", Base, Register);
		CPU::MemBar::Barrier();
		*((volatile uint32_t *)(((uintptr_t)Base))) = Register;
		CPU::MemBar::Barrier();
		uint32_t ret = *((volatile uint32_t *)(((uintptr_t)Base + 16)));
		CPU::MemBar::Barrier();
		return ret;
	}

	void APIC::EOI() { this->Write(APIC_EOI, 0); }

	void APIC::WaitForIPI()
	{
		InterruptCommandRegisterLow icr = {.raw = 0};
		do
		{
			icr.raw = this->Read(APIC_ICRLO);
			CPU::Pause();
		} while (icr.DeliveryStatus != Idle);
	}

	void APIC::IPI(uint8_t CPU, InterruptCommandRegisterLow icr)
	{
		SmartCriticalSection(APICLock);
		if (x2APICSupported)
		{
			wrmsr(MSR_X2APIC_ICR, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
		else
		{
			this->Write(APIC_ICRHI, (CPU << 24));
			this->Write(APIC_ICRLO, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
	}

	void APIC::SendInitIPI(uint8_t CPU)
	{
		SmartCriticalSection(APICLock);
		if (x2APICSupported)
		{
			InterruptCommandRegisterLow icr = {.raw = 0};
			icr.DeliveryMode = INIT;
			icr.Level = Assert;
			wrmsr(MSR_X2APIC_ICR, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
		else
		{
			InterruptCommandRegisterLow icr = {.raw = 0};
			icr.DeliveryMode = INIT;
			icr.Level = Assert;
			this->Write(APIC_ICRHI, (CPU << 24));
			this->Write(APIC_ICRLO, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
	}

	void APIC::SendStartupIPI(uint8_t CPU, uint64_t StartupAddress)
	{
		SmartCriticalSection(APICLock);
		if (x2APICSupported)
		{
			InterruptCommandRegisterLow icr = {.raw = 0};
			icr.Vector = static_cast<uint8_t>(StartupAddress >> 12);
			icr.DeliveryMode = Startup;
			icr.Level = Assert;
			wrmsr(MSR_X2APIC_ICR, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
		else
		{
			InterruptCommandRegisterLow icr = {.raw = 0};
			icr.Vector = static_cast<uint8_t>(StartupAddress >> 12);
			icr.DeliveryMode = Startup;
			icr.Level = Assert;
			this->Write(APIC_ICRHI, (CPU << 24));
			this->Write(APIC_ICRLO, static_cast<uint32_t>(icr.raw));
			this->WaitForIPI();
		}
	}

	uint32_t APIC::IOGetMaxRedirect(uint32_t APICID)
	{
		uint32_t TableAddress = (this->IORead((((ACPI::MADT *)PowerManager->GetMADT())->ioapic[APICID]->Address), GetIOAPICVersion));
		return ((IOAPICVersion *)&TableAddress)->MaximumRedirectionEntry;
	}

	void APIC::RawRedirectIRQ(uint16_t Vector, uint32_t GSI, uint16_t Flags, int CPU, int Status)
	{
		uint64_t Value = Vector;

		int64_t IOAPICTarget = -1;
		for (uint64_t i = 0; ((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(i)] != 0; i++)
			if (((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(i)]->GSIBase <= GSI)
				if (((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(i)]->GSIBase + IOGetMaxRedirect(static_cast<uint32_t>(i)) > GSI)
				{
					IOAPICTarget = i;
					break;
				}

		if (IOAPICTarget == -1)
		{
			error("No ISO table found for I/O APIC");
			return;
		}

		// TODO: IOAPICRedirectEntry Entry = {.raw = 0};

		if (Flags & ActiveHighLow)
			Value |= (1 << 13);

		if (Flags & EdgeLevel)
			Value |= (1 << 15);

		if (!Status)
			Value |= (1 << 16);

		Value |= (((uintptr_t)CPU) << 56);
		uint32_t IORegister = (GSI - ((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(IOAPICTarget)]->GSIBase) * 2 + 16;

		this->IOWrite(((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(IOAPICTarget)]->Address,
					  IORegister, (uint32_t)Value);
		this->IOWrite(((ACPI::MADT *)PowerManager->GetMADT())->ioapic[std::size_t(IOAPICTarget)]->Address,
					  IORegister + 1, (uint32_t)(Value >> 32));
	}

	void APIC::RedirectIRQ(int CPU, uint16_t IRQ, int Status)
	{
		for (uint64_t i = 0; i < ((ACPI::MADT *)PowerManager->GetMADT())->iso.size(); i++)
			if (((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->IRQSource == IRQ)
			{
				debug("[ISO %d] Mapping to source IRQ%#d GSI:%#lx on CPU %d",
					  i, ((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->IRQSource,
					  ((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->GSI,
					  CPU);

				this->RawRedirectIRQ(((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->IRQSource + 0x20,
									 ((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->GSI,
									 ((ACPI::MADT *)PowerManager->GetMADT())->iso[std::size_t(i)]->Flags,
									 CPU, Status);
				return;
			}
		debug("Mapping IRQ%d on CPU %d", IRQ, CPU);
		this->RawRedirectIRQ(IRQ + 0x20, IRQ, 0, CPU, Status);
	}

	void APIC::RedirectIRQs(int CPU)
	{
		SmartCriticalSection(APICLock);
		debug("Redirecting IRQs...");
		for (uint8_t i = 0; i < 16; i++)
			this->RedirectIRQ(CPU, i, 1);
		debug("Redirecting IRQs completed.");
	}

	APIC::APIC(int Core)
	{
		SmartCriticalSection(APICLock);
		APIC_BASE BaseStruct = {.raw = rdmsr(MSR_APIC_BASE)};
		uint64_t BaseLow = BaseStruct.ApicBaseLo;
		uint64_t BaseHigh = BaseStruct.ApicBaseHi;
		this->APICBaseAddress = BaseLow << 12u | BaseHigh << 32u;
		trace("APIC Address: %#lx", this->APICBaseAddress);
		Memory::Virtual().Map((void *)this->APICBaseAddress, (void *)this->APICBaseAddress, Memory::PTFlag::RW | Memory::PTFlag::PCD);

		bool x2APICSupported = false;
		if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
		{
			CPU::x86::AMD::CPUID0x00000001 cpuid;
			if (cpuid.ECX.x2APIC)
			{
				// x2APICSupported = cpuid.ECX.x2APIC;
				fixme("x2APIC is supported");
			}
		}
		else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
		{
			CPU::x86::Intel::CPUID0x00000001 cpuid;
			if (cpuid.ECX.x2APIC)
			{
				// x2APICSupported = cpuid.ECX.x2APIC;
				fixme("x2APIC is supported");
			}
		}

		if (x2APICSupported)
		{
			this->x2APICSupported = true;
			wrmsr(MSR_APIC_BASE, (rdmsr(MSR_APIC_BASE) | (1 << 11)) & ~(1 << 10));
			BaseStruct.EN = 1;
			wrmsr(MSR_APIC_BASE, BaseStruct.raw);
		}
		else
		{
			BaseStruct.EN = 1;
			wrmsr(MSR_APIC_BASE, BaseStruct.raw);
		}

		this->Write(APIC_TPR, 0x0);
		// this->Write(APIC_SVR, this->Read(APIC_SVR) | 0x100); // 0x1FF or 0x100 ? on https://wiki.osdev.org/APIC is 0x100

		if (!this->x2APICSupported)
		{
			this->Write(APIC_DFR, 0xF0000000);
			this->Write(APIC_LDR, this->Read(APIC_ID));
		}

		ACPI::MADT *madt = (ACPI::MADT *)PowerManager->GetMADT();

		for (size_t i = 0; i < madt->nmi.size(); i++)
		{
			if (madt->nmi[std::size_t(i)]->processor != 0xFF && Core != madt->nmi[std::size_t(i)]->processor)
				return;

			uint32_t nmi = 0x402;
			if (madt->nmi[std::size_t(i)]->flags & 2)
				nmi |= 1 << 13;
			if (madt->nmi[std::size_t(i)]->flags & 8)
				nmi |= 1 << 15;
			if (madt->nmi[std::size_t(i)]->lint == 0)
				this->Write(APIC_LINT0, nmi);
			else if (madt->nmi[std::size_t(i)]->lint == 1)
				this->Write(APIC_LINT1, nmi);
		}

		// Setup the spurrious interrupt vector
		Spurious Spurious = {.raw = this->Read(APIC_SVR)};
		Spurious.Vector = IRQ223; // TODO: Should I map the IRQ to something?
		Spurious.Software = 1;
		this->Write(APIC_SVR, static_cast<uint32_t>(Spurious.raw));

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

	APIC::~APIC() {}

	int Timer::OnInterruptReceived(CPU::TrapFrame *Frame)
	{
		UNUSED(Frame);
		return EOK;
	}

	void Timer::OneShot(uint32_t Vector, uint64_t Miliseconds)
	{
		SmartCriticalSection(APICLock);
		LVTTimer timer = {.raw = 0};
		timer.Vector = static_cast<uint8_t>(Vector);
		timer.TimerMode = 0;
		if (strcmp(CPU::Hypervisor(), x86_CPUID_VENDOR_TCG) != 0)
			this->lapic->Write(APIC_TDCR, DivideBy128);
		else
			this->lapic->Write(APIC_TDCR, DivideBy16);
		this->lapic->Write(APIC_TICR, static_cast<uint32_t>(Ticks * Miliseconds));
		this->lapic->Write(APIC_TIMER, static_cast<uint32_t>(timer.raw));
	}

	Timer::Timer(APIC *apic) : Interrupt::Handler(0) /* IRQ0 */
	{
		SmartCriticalSection(APICLock);
		this->lapic = apic;
		LVTTimerDivide Divider = DivideBy16;

		trace("Initializing APIC timer on CPU %d", GetCurrentCPU()->ID);

		this->lapic->Write(APIC_TDCR, Divider);
		this->lapic->Write(APIC_TICR, 0xFFFFFFFF);

		TimeManager->Sleep(Time::FromMilliseconds(1));

		// Mask the timer
		this->lapic->Write(APIC_TIMER, 0x10000 /* LVTTimer.Mask flag */);
		Ticks = 0xFFFFFFFF - this->lapic->Read(APIC_TCCR);

		// Config for IRQ0 timer
		LVTTimer timer = {.raw = 0};
		timer.Vector = IRQ0;
		timer.Mask = Unmasked;
		timer.TimerMode = LVTTimerMode::OneShot;

		// Initialize APIC timer
		this->lapic->Write(APIC_TDCR, Divider);
		this->lapic->Write(APIC_TICR, static_cast<uint32_t>(Ticks));
		this->lapic->Write(APIC_TIMER, static_cast<uint32_t>(timer.raw));
		trace("%d APIC Timer %d ticks in.", GetCurrentCPU()->ID, Ticks);
		klog("APIC Timer: \x1b[1;32m%ld\x1b[0m ticks.", Ticks);
	}

	Timer::~Timer()
	{
	}
}
