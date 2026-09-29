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

#include "hpet.hpp"

#include <memory.hpp>
#include <acpi.hpp>
#include <debug.h>
#include <io.h>

namespace Platform
{
	uint64_t HighPrecisionEventTimer::Now() const
	{
		uint64_t counter = mminq(&this->hpet->MainCounter);

		/* convert ticks to nanoseconds: counter * period_fs / 1e6 */
		return (counter * 1'000'000'000ULL) / this->clk;
		// return mminq(&this->hpet->MainCounter);
	}

	void HighPrecisionEventTimer::Sleep(std::chrono::nanoseconds ns)
	{
		uint64_t target = this->Now() + ns.count();
		while (this->Now() < target)
			CPU::Pause();
	}

	void HighPrecisionEventTimer::SetOneShot(std::chrono::nanoseconds ns, Interrupt::IRQLines Line)
	{
		assert(!"not implemented");
	}

	void HighPrecisionEventTimer::SetPeriodic(std::chrono::nanoseconds ns, Interrupt::IRQLines Line)
	{
		assert(!"not implemented");
	}

	HighPrecisionEventTimer::HighPrecisionEventTimer(ACPI::HPETHeader *_hpet)
	{
		Memory::Virtual().SingleMap(_hpet->Address.Address, _hpet->Address.Address, Memory::RW | Memory::PCD | Memory::PWT);
		this->hpet = reinterpret_cast<HPET *>(_hpet->Address.Address);
		debug("%s timer is at address %#lx", _hpet->Header.OEMID, _hpet->Address.Address);
		uint64_t period_fs = this->hpet->CapabilitiesID >> 32;
		assert(period_fs != 0);

		/* Hz = 1e15 / period_fs */
		this->clk = 1'000'000'000'000'000ULL / period_fs;
		klog("HPET tick period: %lu femtoseconds -> %u Hz", period_fs, this->clk);

		mmoutq(&this->hpet->Configuration, 0);
		mmoutq(&this->hpet->MainCounter, 0);
		mmoutq(&this->hpet->Configuration, 1);

		uint64_t cfg = mminq(&this->hpet->Configuration);
		if (!(cfg & 1))
			assert(!"HPET counter is not enabled!");
	}

	HighPrecisionEventTimer::~HighPrecisionEventTimer()
	{
		mmoutq(&this->hpet->Configuration, 0);
	}
}
