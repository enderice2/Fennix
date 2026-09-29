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

#include <time.hpp>
#include <debug.h>
#include <io.h>

namespace Time
{
	Clock ReadClock()
	{
		Clock tm;
		uint32_t t = 0;
		outb(0x70, 0x00);
		t = inb(0x71);
		tm.Second = ((t & 0x0F) + ((t >> 4) * 10));
		outb(0x70, 0x02);
		t = inb(0x71);
		tm.Minute = ((t & 0x0F) + ((t >> 4) * 10));
		outb(0x70, 0x04);
		t = inb(0x71);
		tm.Hour = ((t & 0x0F) + ((t >> 4) * 10));
		outb(0x70, 0x07);
		t = inb(0x71);
		tm.Day = ((t & 0x0F) + ((t >> 4) * 10));
		outb(0x70, 0x08);
		t = inb(0x71);
		tm.Month = ((t & 0x0F) + ((t >> 4) * 10));
		outb(0x70, 0x09);
		t = inb(0x71);
		tm.Year = ((t & 0x0F) + ((t >> 4) * 10));
		tm.Counter = 0;
		return tm;
	}

	Clock ConvertFromUnix(uint64_t Timestamp)
	{
		Clock result;

		uint64_t Seconds = Timestamp;
		uint64_t Minutes = Seconds / 60;
		uint64_t Hours = Minutes / 60;
		uint64_t Days = Hours / 24;

		result.Year = 1970;
		while (Days >= 365)
		{
			if (result.Year % 4 == 0 &&
				(result.Year % 100 != 0 ||
				 result.Year % 400 == 0))
			{
				if (Days >= 366)
				{
					Days -= 366;
					result.Year++;
				}
				else
					break;
			}
			else
			{
				Days -= 365;
				result.Year++;
			}
		}

		int DaysInMonth[] = {31,
							 result.Year % 4 == 0
								 ? 29
								 : 28,
							 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		for (result.Month = 0; result.Month < 12; result.Month++)
		{
			if (Days < static_cast<uint64_t>((DaysInMonth[result.Month])))
				break;
			Days -= DaysInMonth[result.Month];
		}
		result.Month++;

		result.Day = static_cast<int>((Days) + 1);
		result.Hour = static_cast<int>((Hours % 24));
		result.Minute = static_cast<int>((Minutes % 60));
		result.Second = static_cast<int>((Seconds % 60));
		result.Counter = static_cast<uint64_t>((Timestamp));
		return result;
	}
}
