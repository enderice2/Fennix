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

#if defined(__amd64__) || defined(__i386__)

#include "uhci.hpp"

#include <cpu.hpp>
#include <pci.hpp>
#include <foward_list>

#include "../../../../kernel.h"

extern PCI::Manager *PCIManager;

namespace Driver::UniversalHostControllerInterface
{
	static_assert(sizeof(TD) == 32, "TD must be 32 bytes");
	static_assert(sizeof(QH) == 64, "QH must be 64 bytes");

	dev_t DriverID;

	int devStart(struct USBController *d) { return ((HCD *)d)->Start(); }
	int devStop(struct USBController *d) { return ((HCD *)d)->Stop(); }
	int devReset(struct USBController *d) { return ((HCD *)d)->Reset(); }
	int devSubmit(struct USBDevice *dev, struct USBRequestBlock *urb) { return ((HCD *)dev->Controller)->Submit(dev, urb); }
	int devCancel(struct USBDevice *dev, struct USBRequestBlock *urb) { return ((HCD *)dev->Controller)->Cancel(dev, urb); }

	std::list<PCI::PCIDevice> Devices;
	std::list<HCD *> Controllers;
	int Entry()
	{
		for (auto &&dev : Devices)
		{
			PCIManager->InitializeDevice(dev, KernelPageTable);

			HCD *hc = new HCD(dev);
			hc->StartController = devStart;
			hc->StopController = devStop;
			hc->ResetController = devReset;
			hc->SubmitURB = devSubmit;
			hc->CancelURB = devCancel;

			int ret = hc->Reset();
			if (ret != 0)
			{
				klog("Failed to reset UHCI controller %d:%d:%d: %s", dev.Bus, dev.Device, dev.Function, strerror(ret));
				delete hc;
				continue;
			}
			ret = hc->Start();
			if (ret != 0)
			{
				klog("Failed to start UHCI controller %d:%d:%d: %s", dev.Bus, dev.Device, dev.Function, strerror(ret));
				delete hc;
				continue;
			}
			ret = hc->Detect();
			if (ret != 0)
			{
				klog("Failed to detect UHCI devices on controller %d:%d:%d: %s", dev.Bus, dev.Device, dev.Function, strerror(ret));
				delete hc;
				continue;
			}

			Controllers.push_back(hc);
			v0::AddController(DriverID, hc);
		}

		return 0;
	}

	int Final()
	{
		for (auto &&hc : Controllers)
		{
			v0::RemoveController(DriverID, hc);
			delete hc;
		}
		return 0;
	}

	int Panic()
	{
		for (auto &&i : Controllers)
			i->Stop();

		return EOK;
	}

	int Probe()
	{
		Devices = PCIManager->FindPCIDevice(0xC, 0x3, 0x00);
		if (Devices.empty())
			return ENODEV;
		debug("there are %d controllers", Devices.size());
		return 0;
	}

	REGISTER_BUILTIN_DRIVER(uhci,
							"Universal Host Controller Interface Driver",
							"enderice2",
							1, 0, 0,
							Entry,
							Final,
							Panic,
							Probe);
}

#endif
