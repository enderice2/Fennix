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

#pragma once

#include <time.hpp>
#include <acpi.hpp>

namespace Platform
{
	class HighPrecisionEventTimer : public Time::ClockSource, public Time::TimerDevice
	{
	private:
		struct HPET
		{
			uint64_t CapabilitiesID;
			uint64_t __reserved0;
			uint64_t Configuration;
			uint64_t __reserved1;
			uint64_t InterruptStatus;
			uint64_t __reserved2[25];
			uint64_t MainCounter;
			uint64_t __reserved3;
		};

		uint64_t clk = 0;
		HPET *hpet = nullptr;

	public:
		const char *Name() const final { return "HPET"; }

		uint64_t Now() const final;

		uint64_t Frequency() const final { return clk; }

		void Sleep(std::chrono::nanoseconds ns) final;

		void SetOneShot(std::chrono::nanoseconds ns, Interrupt::IRQLines Line) final;
		void SetPeriodic(std::chrono::nanoseconds ns, Interrupt::IRQLines Line) final;

		HighPrecisionEventTimer(ACPI::HPETHeader *hpet);
		~HighPrecisionEventTimer();
	};
}
