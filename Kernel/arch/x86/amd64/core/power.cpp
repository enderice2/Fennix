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

#include <power.hpp>

#include <entry.hpp>
#include <memory.hpp>
#include <entry.hpp>
#include <debug.h>
#include <io.h>

#include "../kernel.h"

namespace Power
{
	void Power::arch_reboot()
	{
		if (ACPIManager && DSDTManager)
		{
			if (ACPIManager->FADT)
			{
				if (DSDTManager->ACPIShutdownSupported)
				{
					klog("Waiting for ACPI...");
					DSDTManager->Reboot();
				}
			}
		}

		asmv("cli");

		uint8_t temp;
		do
		{
			temp = inb(0x64);
			if (((temp) & (1 << (0))) != 0)
				inb(0x60);
		} while (((temp) & (1 << (1))) != 0);

		outb(0x64, 0xFE);
	}

	void Power::arch_shutdown()
	{
		if (ACPIManager && DSDTManager)
		{
			if (ACPIManager->FADT)
			{
				if (DSDTManager->ACPIShutdownSupported)
				{
					klog("Waiting for ACPI...");
					DSDTManager->Shutdown();
				}
				else
					error("Shutdown not supported");

				/* TODO: If no ACPI, display "It is now safe to turn off your computer"? */
				assert(!"It is now safe to turn off your computer");
			}
		}

		/* FIXME: Detect emulators and use their shutdown methods */
#ifdef DEBUG
		outl(0xB004, 0x2000); // for qemu
		outl(0x604, 0x2000);  // if qemu not working, bochs and older versions of qemu
		outl(0x4004, 0x3400); // virtual box
#endif
	}

	void Power::arch_init()
	{
	}

	Power::Power()
	{
		debug("Power manager initialized");
	}
}
