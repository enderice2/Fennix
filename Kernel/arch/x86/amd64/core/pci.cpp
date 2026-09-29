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

#include <pci.hpp>

#include <entry.hpp>
#include <power.hpp>
#include <acpi.hpp>
#include <quirks.hpp>

#include "../kernel.h"

namespace PCI
{
	Manager::Manager()
	{
#if defined(__amd64__) || defined(__i386__)

		if (!ACPIManager)
		{
			error("ACPI not found");
			return;
		}

		Platform::ACPI::MCFGHeader *mcfg = ACPIManager->MCFG;
		if (!mcfg)
		{
			error("MCFG not found");
			return;
		}

		int Entries = static_cast<int>(((mcfg->Header.Length) - sizeof(Platform::ACPI::MCFGHeader)) / sizeof(DeviceConfig));
		Memory::Virtual vmm(KernelPageTable);
		for (int t = 0; t < Entries; t++)
		{
			DeviceConfig *NewDeviceConfig = (DeviceConfig *)((uintptr_t)mcfg + sizeof(Platform::ACPI::MCFGHeader) + (sizeof(DeviceConfig) * t));
			vmm.SingleMap((void *)NewDeviceConfig->BaseAddress, (void *)NewDeviceConfig->BaseAddress, Memory::PTFlag::RW);
			debug("PCI Entry %d Address:%p BUS:%#x-%#x", t, NewDeviceConfig->BaseAddress,
				  NewDeviceConfig->StartBus, NewDeviceConfig->EndBus);

			PCIDevice dev{};
			dev.Config = NewDeviceConfig;
			for (uint32_t Bus = NewDeviceConfig->StartBus; Bus < NewDeviceConfig->EndBus; Bus++)
				EnumerateBus(NewDeviceConfig->BaseAddress, Bus, dev);
		}
#elif defined(__aarch64__)
		error("PCI not implemented on aarch64");
#endif
	}

	Manager::~Manager() {}
}
