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

#include <usb.hpp>
#include <auto_page>

#include "../../../../kernel.h"

namespace UniversalSerialBus
{
	InputReport kir = {};
	int ReportKeyboardEvent(uint32_t Key, uint8_t Pressed)
	{
		kir.Type = INPUT_TYPE_KEYBOARD;
		// kir.Device = Device;
		kir.Keyboard.Key = (KeyScanCodes)(Key);
		kir.Keyboard.Pressed = Pressed;
		// v0::ReportInputEvent(DriverID, &kir);
		return 0;
	}

	void OnURBCompleteKeyboard(struct USBRequestBlock *urb)
	{
		if (urb->Status != USB_REQ_SUCCESS)
		{
			urb->Device->SubmitRequest(urb);
			return;
		}

		uint8_t *buf = (uint8_t *)urb->Buffer;

		debug("Keyboard raw data: %02x %02x %02x %02x %02x %02x %02x %02x",
			  buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

		uint8_t *data = buf;
		bool error = false;
		static uint8_t lastData[8];

		// Modifier keys
		uint32_t modDelta = data[0] ^ lastData[0];
		for (uint32_t i = 0; i < 8; ++i)
		{
			uint32_t mask = 1 << i;
			if (modDelta & mask)
			{
				klog("Modifier key %#lx %s", mask << 8, data[0] & mask ? "pressed" : "released");
			}
		}

		// Release keys
		for (uint32_t i = 2; i < 8; ++i)
		{
			uint32_t usage = lastData[i];

			if (usage)
			{
				if (!memchr(data + 2, usage, 6))
				{
					klog("Key %#lx released", usage);
				}
			}
		}

		// Press keys
		for (uint32_t i = 2; i < 8; ++i)
		{
			uint32_t usage = data[i];

			if (usage >= 4)
			{
				if (!memchr(lastData + 2, usage, 6))
				{
					klog("Key %#lx pressed", usage);
				}
			}
			else if (usage > 0)
			{
				error = true;
			}
		}

		// Update keystate
		if (!error)
		{
			memcpy(lastData, data, 8);
		}

		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		buf[4] = 0;
		buf[5] = 0;
		buf[6] = 0;
		buf[7] = 0;

		urb->Device->SubmitRequest(urb);
	}

	int InitializeKeyboard(USBDevice *Device)
	{
		if (Device->Interface.bInterfaceClass != USB_CLASS_HID ||
			Device->Interface.bInterfaceSubClass != USB_HID_SUBCLASS_BOOT ||
			Device->Interface.bInterfaceProtocol != USB_HID_PROTOCOL_KEYBOARD)
			return -ENODEV;

		USBDeviceRequest *req = (USBDeviceRequest *)KernelAllocator.RequestPages(TO_PAGES(sizeof(USBDeviceRequest)));
		Memory::Virtual().Map(req, req, sizeof(USBDeviceRequest), Memory::P | Memory::RW | Memory::PWT | Memory::PCD);
		req->bmRequestType.Recipient = 0b1;
		req->bmRequestType.Type = 0b01;
		req->bmRequestType.Direction = 0b0;
		req->bRequest = 0xA; /* SET IDLE REQUEST */
		req->wValue = 0;
		req->wIndex = Device->Interface.bInterfaceNumber;
		req->wLength = 0;
		int result = usb->RequestDevice(Device, req, 0);
		if (result != 0)
		{
			debug("failed to set idle: %d", result);
			return result;
		}

		USBRequestBlock *urb = (USBRequestBlock *)KernelAllocator.RequestPages(TO_PAGES(sizeof(USBRequestBlock)));
		Memory::Virtual().Map(urb, urb, sizeof(USBRequestBlock), Memory::P | Memory::RW | Memory::PWT | Memory::PCD);
		urb->Buffer = KernelAllocator.RequestPage();
		Memory::Virtual().Map(urb->Buffer, urb->Buffer, PAGE_SIZE, Memory::P | Memory::RW | Memory::PWT | Memory::PCD);
		urb->Device = Device;
		urb->Complete = OnURBCompleteKeyboard;
		urb->EndpointAddress = Device->Endpoints[1].Address;
		urb->Length = Device->Endpoints[1].MaxPacketSize;
		urb->Request = nullptr;
		urb->Type = USB_TRANSFER_INTERRUPT;
		// Device->SubmitRequest(urb);
		return 0;
	}
}
