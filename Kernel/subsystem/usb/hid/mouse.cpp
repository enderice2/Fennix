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
	InputReport mir = {};
	int ReportMouseEvent(uint32_t Key, uint8_t Pressed)
	{
		mir.Type = INPUT_TYPE_MOUSE;
		// mir.Device = MouseDevID;
		// mir.Mouse.LeftButton = Packet.Base.LeftButton;
		// mir.Mouse.RightButton = Packet.Base.RightButton;
		// mir.Mouse.MiddleButton = Packet.Base.MiddleButton;
		// mir.Mouse.Button4 = Packet.ZMovement.Button4;
		// mir.Mouse.Button5 = Packet.ZMovement.Button5;
		// mir.Mouse.X = X;
		// mir.Mouse.Y = -Y;
		// mir.Mouse.Z = Packet.ZMovement.Z;
		// v0::ReportInputEvent(DriverID, &mir);
		return 0;
	}

	void OnURBCompleteMouse(struct USBRequestBlock *urb)
	{
		if (urb->Status != USB_REQ_SUCCESS)
		{
			urb->Device->SubmitRequest(urb);
			return;
		}

		static uint32_t oldcol = 0;
		static int mx = 0, my = 0;
		uint8_t *buf = (uint8_t *)urb->Buffer;
		debug("%c%c%c dx=%d dy=%d",
			  buf[0] & 0x1 ? 'L' : ' ',
			  buf[0] & 0x2 ? 'R' : ' ',
			  buf[0] & 0x4 ? 'M' : ' ',
			  (int8_t)buf[1],
			  (int8_t)buf[2]);

		Display->SetPixel(mx, my, oldcol);

		// Update mouse position with bounds checking
		mx += (int8_t)buf[1];
		my += (int8_t)buf[2];

		int width = Display->GetWidth;
		int height = Display->GetHeight;

		if (mx < 0)
			mx = 0;
		if (my < 0)
			my = 0;
		if (mx >= width)
			mx = width - 1;
		if (my >= height)
			my = height - 1;

		uint32_t color;
		if (buf[0] & 0x1)
			color = 0xff0000; // Left button: red
		else if (buf[0] & 0x2)
			color = 0x00ff00; // Right button: green
		else if (buf[0] & 0x4)
			color = 0x0000ff; // Middle button: blue
		else
			color = 0xffffff; // No button: white

		oldcol = Display->GetPixel(mx, my);
		Display->SetPixel(mx, my, color);
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;

		urb->Device->SubmitRequest(urb);
	}

	int InitializeMouse(USBDevice *Device)
	{
		if (Device->Interface.bInterfaceClass != USB_CLASS_HID ||
			Device->Interface.bInterfaceSubClass != USB_HID_SUBCLASS_BOOT ||
			Device->Interface.bInterfaceProtocol != USB_HID_PROTOCOL_MOUSE)
			return -ENODEV;

		USBRequestBlock *urb = (USBRequestBlock *)KernelAllocator.RequestPages(TO_PAGES(sizeof(USBRequestBlock)));
		Memory::Virtual().Map(urb, urb, sizeof(USBRequestBlock), Memory::P | Memory::RW | Memory::PWT | Memory::PCD);
		urb->Buffer = KernelAllocator.RequestPage();
		Memory::Virtual().Map(urb->Buffer, urb->Buffer, PAGE_SIZE, Memory::P | Memory::RW | Memory::PWT | Memory::PCD);
		urb->Device = Device;
		urb->Complete = OnURBCompleteMouse;
		urb->EndpointAddress = Device->Endpoints[1].Address;
		urb->Length = Device->Endpoints[1].MaxPacketSize;
		urb->Request = nullptr;
		urb->Type = USB_TRANSFER_INTERRUPT;
		// Device->SubmitRequest(urb);
		return 0;
	}
}
