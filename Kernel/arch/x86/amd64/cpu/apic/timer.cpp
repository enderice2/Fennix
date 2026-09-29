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
#include <cpu/x86/msr.hpp>

#include <smp.hpp>
#include <io.h>
#include <entry.hpp>

using namespace CPU::x86;
using namespace std::chrono_literals;

namespace APIC
{
	namespace
	{
		uint32_t ReadCurrentCount(Local *lapic)
		{
			if (lapic->x2APIC)
				return static_cast<uint32_t>(rdmsr(MSR_X2APIC_CUR_COUNT));
			return lapic->Read(APIC_TCCR);
		}

		void WriteLvtTimer(Local *lapic, uint32_t Value)
		{
			if (lapic->x2APIC)
				wrmsr(MSR_X2APIC_LVT_TIMER, Value);
			else
				lapic->Write(APIC_TIMER, Value);
		}

		void WriteInitCount(Local *lapic, uint32_t Value)
		{
			if (lapic->x2APIC)
				wrmsr(MSR_X2APIC_INIT_COUNT, Value);
			else
				lapic->Write(APIC_TICR, Value);
		}

		void WriteDivider(Local *lapic, LVTTimerDivide Divider)
		{
			if (lapic->x2APIC)
				wrmsr(MSR_X2APIC_DIV_CONF, Divider);
			else
				lapic->Write(APIC_TDCR, Divider);
		}

		uint64_t CalibrateTicksPerSecond(Local *lapic, LVTTimerDivide Divider,
										 std::chrono::milliseconds Window,
										 bool Unmask)
		{
			if (!GlobalClock)
				return 0;

			LVTTimer lvt{};
			lvt.VEC = CPU::x86::IRQ0;
			lvt.M = Unmask ? Unmasked : Masked;
			lvt.TMM = OneShot;

			WriteLvtTimer(lapic, lvt.raw);
			WriteDivider(lapic, Divider);
			WriteInitCount(lapic, 0xFFFFFFFFu);

			GlobalClock->Sleep(Window);

			uint32_t current = ReadCurrentCount(lapic);
			uint64_t delta = 0xFFFFFFFFu - current;
			if (delta == 0)
				return 0;

			uint64_t scale = 1000 / uint64_t(Window.count());
			return delta * scale;
		}
	}

	void Timer::SetOneShot(std::chrono::nanoseconds ns, Interrupt::IRQLines Line)
	{
		uint64_t ticks = 0;
		if (ns.count() > 0 && TicksPerSecond > 0)
		{
			__uint128_t prod = (__uint128_t)ns.count() * TicksPerSecond;
			ticks = (uint64_t)(prod / 1'000'000'000ULL);
		}

		if (ticks > 0xFFFFFFFFu)
			ticks = 0xFFFFFFFFu;

		if (ticks == 0) /* Minimum 1 tick */
			ticks = 1;

		LVTTimer lvt{};
		lvt.VEC = uint8_t(Line + (int)CPU::x86::IRQ0);
		lvt.M = Unmasked;
		lvt.TMM = OneShot;

		if (lapic->x2APIC)
		{
			wrmsr(MSR_X2APIC_LVT_TIMER, lvt.raw);
			wrmsr(MSR_X2APIC_INIT_COUNT, (uint32_t)ticks);
		}
		else
		{
			lapic->Write(APIC_TIMER, lvt.raw);
			lapic->Write(APIC_TICR, (uint32_t)ticks);
		}

		// /* FIXME: Sometimes APIC stops firing when debugging, why? */
		// LVTTimer timer{};
		// timer.VEC = uint8_t(Line + CPU::x86::IRQ0);
		// timer.TMM = LVTTimerMode::OneShot;

		// LVTTimerDivide Divider = DivideBy8;

		// if (this->lapic->x2APIC)
		// {
		// 	// wrmsr(MSR_X2APIC_DIV_CONF, Divider); <- gpf on real hardware
		// 	wrmsr(MSR_X2APIC_INIT_COUNT, uint32_t(TicksPerSecond * std::chrono::duration_cast<std::chrono::milliseconds>(ns).count()));
		// 	wrmsr(MSR_X2APIC_LVT_TIMER, uint32_t(timer.raw));
		// }
		// else
		// {
		// 	this->lapic->Write(APIC_TDCR, Divider);
		// 	this->lapic->Write(APIC_TICR, uint32_t(TicksPerSecond * std::chrono::duration_cast<std::chrono::milliseconds>(ns).count()));
		// 	this->lapic->Write(APIC_TIMER, uint32_t(timer.raw));
		// }
	}

	void Timer::SetPeriodic(std::chrono::nanoseconds ns, Interrupt::IRQLines Line)
	{
		uint64_t ticks = 0;
		if (ns.count() > 0 && TicksPerSecond > 0)
		{
			__uint128_t prod = (__uint128_t)ns.count() * TicksPerSecond;
			ticks = (uint64_t)(prod / 1'000'000'000ULL);
		}

		if (ticks > 0xFFFFFFFFu)
			ticks = 0xFFFFFFFFu;

		if (ticks == 0)
			ticks = 1;

		LVTTimer lvt{};
		lvt.VEC = uint8_t(Line + (int)CPU::x86::IRQ0);
		lvt.M = Unmasked;
		lvt.TMM = Periodic;

		if (lapic->x2APIC)
		{
			wrmsr(MSR_X2APIC_LVT_TIMER, lvt.raw);
			wrmsr(MSR_X2APIC_INIT_COUNT, (uint32_t)ticks);
		}
		else
		{
			lapic->Write(APIC_TIMER, lvt.raw);
			lapic->Write(APIC_TICR, (uint32_t)ticks);
		}
	}

	Timer::Timer(Local *local) : lapic(local)
	{
		debug("Initializing APIC timer on CPU %d", GetCurrentCPU()->ID);
		CriticalSection cs;

		// if (this->lapic->x2APIC)
		// {
		// 	wrmsr(MSR_X2APIC_DIV_CONF, 1);
		// 	wrmsr(MSR_X2APIC_INIT_COUNT, 0);
		// }
		// else
		// {
		// 	/* Mask the timer IRQ */
		// 	uint32_t lvt = this->lapic->Read(APIC_TIMER);
		// 	lvt |= (1 << 16);
		// 	this->lapic->Write(APIC_TIMER, lvt);

		// 	this->lapic->Write(APIC_TIMER, CPU::x86::IRQ0);
		// 	this->lapic->Write(APIC_TDCR, 1);
		// 	this->lapic->Write(APIC_TICR, 0);
		// }

		// if (this->lapic->x2APIC)
		// 	wrmsr(MSR_X2APIC_INIT_COUNT, 0xFFFFFFFF);

		// else
		// 	this->lapic->Write(APIC_TICR, 0xFFFFFFFF);

		// /* ... */

		// if (this->lapic->x2APIC)
		// {
		// 	wrmsr(MSR_X2APIC_INIT_COUNT, 0);
		// }
		// else
		// {
		// 	this->lapic->Write(APIC_TICR, 0);

		// 	/* Unmask the timer IRQ */
		// 	uint32_t lvt = this->lapic->Read(APIC_TIMER);
		// 	lvt &= ~(1 << 16);
		// 	this->lapic->Write(APIC_TIMER, lvt);
		// }

		LVTTimerDivide divider = DivideBy4;
		TicksPerSecond = CalibrateTicksPerSecond(this->lapic, divider, 4ms, false);
		if (TicksPerSecond == 0)
			TicksPerSecond = CalibrateTicksPerSecond(this->lapic, divider, 10ms, true);
		if (TicksPerSecond == 0)
			warn("APIC timer calibration failed on CPU %d", GetCurrentCPU()->ID);

		// Config for IRQ0 timer
		LVTTimer timer{};
		timer.VEC = IRQ0 + CPU::x86::IRQ0;
		timer.M = Unmasked;
		timer.TMM = OneShot;

		// Initialize APIC timer
		if (this->lapic->x2APIC)
		{
			WriteDivider(this->lapic, divider);
			WriteInitCount(this->lapic, (uint32_t)TicksPerSecond);
			WriteLvtTimer(this->lapic, timer.raw);
		}
		else
		{
			WriteDivider(this->lapic, divider);
			WriteInitCount(this->lapic, (uint32_t)TicksPerSecond);
			WriteLvtTimer(this->lapic, uint32_t(timer.raw));
		}

		debug("%d APIC Timer %d ticks in", GetCurrentCPU()->ID, TicksPerSecond);
	}

	Timer::~Timer()
	{
		// Mask the timer
		if (this->lapic->x2APIC)
		{
			wrmsr(MSR_X2APIC_LVT_TIMER, 0x10000 /* LVTTimer.Mask flag */);
		}
		else
		{
			this->lapic->Write(APIC_TIMER, 0x10000 /* LVTTimer.Mask flag */);
		}
	}
}
