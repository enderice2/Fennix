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

#include <uart.hpp>
#include <io.h>

namespace UART
{
	enum SerialPorts
	{
		COM1 = 0x3F8,
		COM2 = 0x2F8,
		COM3 = 0x3E8,
		COM4 = 0x2E8,
		COM5 = 0x5F8,
		COM6 = 0x4F8,
		COM7 = 0x5E8,
		COM8 = 0x4E8
	};

#define SERIAL_ENABLE_DLAB 0x80
#define SERIAL_RATE_115200_LO 0x01
#define SERIAL_RATE_115200_HI 0x00
#define SERIAL_RATE_57600_LO 0x02
#define SERIAL_RATE_57600_HI 0x00
#define SERIAL_RATE_38400_LO 0x03
#define SERIAL_RATE_38400_HI 0x00
#define SERIAL_BUFFER_EMPTY 0x20

	void Driver::DebugWrite(uint8_t Char)
	{
		if (!DebugAvailable)
			return;
		while ((inb(static_cast<uint16_t>(COM1 + 5)) & SERIAL_BUFFER_EMPTY) == 0)
			;
		outb(COM1, Char);
	}

	uint8_t Driver::DebugRead()
	{
		if (!DebugAvailable)
			return 0;
		while ((inb(static_cast<uint16_t>(COM1 + 5)) & 1) == 0)
			;
		return inb(COM1);
	}

	Driver::Driver()
	{
		auto initPort = [](uint16_t Port)
		{
			// Initialize the serial port
			outb(static_cast<uint16_t>(Port + 1), 0x00);				  // Disable all interrupts
			outb(static_cast<uint16_t>(Port + 3), SERIAL_ENABLE_DLAB);	  // Enable DLAB (set baud rate divisor)
			outb(static_cast<uint16_t>(Port + 0), SERIAL_RATE_115200_LO); // Set divisor to 1 (lo byte) 115200 baud
			outb(static_cast<uint16_t>(Port + 1), SERIAL_RATE_115200_HI); //                  (hi byte)
			outb(static_cast<uint16_t>(Port + 3), 0x03);				  // 8 bits, no parity, one stop bit
			outb(static_cast<uint16_t>(Port + 2), 0xC7);				  // Enable FIFO, clear them, with 14-byte threshold
			outb(static_cast<uint16_t>(Port + 4), 0x0B);				  // IRQs enabled, RTS/DSR set

			/* FIXME  https://wiki.osdev.org/Serial_Ports */
			// outb(static_cast<uint16_t>(Port + 0), 0x1E);
			// outb(static_cast<uint16_t>(Port + 0), 0xAE);
			// Check if the serial port is faulty.
			// if (inb(static_cast<uint16_t>(Port + 0)) != 0xAE)
			// {
			//     static int once = 0;
			//     if (!once++)
			//         warn("Serial port %#lx is faulty.", Port);
			//     // serialports[Port] = false; // ignore for now
			//     // return;
			// }

			// Set to normal operation mode.
			outb(static_cast<uint16_t>(Port + 4), 0x0F);
		};

		uint8_t com = inb(COM1);
		if (com != 0xFF)
		{
			initPort(COM1);
			DebugAvailable = true;
		}
	}

	Driver::~Driver() {}
}
