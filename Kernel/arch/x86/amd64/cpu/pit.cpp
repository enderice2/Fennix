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

#include "pit.hpp"

#include <io.h>

#define PIT_CH0_DATA 0x40
#define PIT_COMMAND 0x43
#define PIT_PORT61 0x61

namespace ProgrammableIntervalTimer
{
	uint64_t PIT::Now() const
	{
		return 0;
	}

	void PIT::Sleep(std::chrono::nanoseconds ns)
	{
		uint16_t count = (uint16_t)((clk / 1000) * std::chrono::milliseconds(ns).count());

		outb(PIT_COMMAND, 0b00110000);

		outb(PIT_CH0_DATA, count & 0xFF);
		outb(PIT_CH0_DATA, (count >> 8) & 0xFF);

		uint8_t tmp = inb(PIT_PORT61);
		outb(PIT_PORT61, tmp & ~1);

		while (!(inb(PIT_PORT61) & 0x20))
			asmv("nop");
	}
}
