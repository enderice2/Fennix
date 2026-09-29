/*
	This file is part of Fennix Drivers.

	Fennix Drivers is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix Drivers is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix Drivers. If not, see <https://www.gnu.org/licenses/>.
*/

#if defined(__amd64__) || defined(__i386__)

#include "aip.hpp"

#include <driver.hpp>
#include <interface/input.h>
#include <cpu.hpp>
#include <io.h>

extern const unsigned short ScanCodeSet1[];
extern const unsigned short ScanCodeSet1mm[];
extern const unsigned short ScanCodeSet3[];

namespace Driver::AdvancedIntegratedPeripheral
{
	extern dev_t DriverID;
	uint8_t KeyboardScanCodeSet = 0;
	dev_t KeyboardDevID = -1;

	InputReport kir = {};
	int ReportKeyboardEvent(dev_t Device, const unsigned short ScanCode, uint8_t Pressed)
	{
		kir.Type = INPUT_TYPE_KEYBOARD;
		kir.Device = Device;
		kir.Keyboard.Key = (KeyScanCodes)(ScanCode);
		kir.Keyboard.Pressed = Pressed;
		v0::ReportInputEvent(DriverID, &kir);
		return 0;
	}

	bool IsE0 = false;
	bool IsE1 = false;
	void PS2KbdInterruptHandler(CPU::TrapFrame *)
	{
		uint8_t sc = inb(PS2_DATA);
		if (sc == PS2_KBD_RESP_ACK ||
			sc == PS2_KBD_RESP_ECHO ||
			sc == PS2_KBD_RESP_RESEND)
			return;

		if (sc == 0xE0)
		{
			IsE0 = true;
			return;
		}

		if (sc == 0xE1)
		{
			IsE1 = true;
			return;
		}

		switch (KeyboardScanCodeSet)
		{
		case PS2_KBD_SC_SET_1:
		case PS2_KBD_SC_SET_2:
		{
			if (IsE0)
			{
				IsE0 = false;
				ReportKeyboardEvent(KeyboardDevID, ScanCodeSet1mm[sc], sc < 0x90);
				return;
			}
			else
			{
				bool released = sc & 0x80;
				uint8_t scFinal = released ? sc & 0x7F : sc;
				ReportKeyboardEvent(KeyboardDevID, ScanCodeSet1[scFinal], !released);
				return;
			}
		}
		/* FIXME: https://wiki.osdev.org/PS/2_Keyboard */
		// case PS2_KBD_SC_SET_2:
		// {
		// 	break;
		// }
		case PS2_KBD_SC_SET_3:
		{
			ReportKeyboardEvent(KeyboardDevID, ScanCodeSet3[sc], true);
			ReportKeyboardEvent(KeyboardDevID, ScanCodeSet3[sc], false);
			break;
		}
		default:
		{
			if (IsE0)
				IsE0 = false;
			trace("Unknown PS/2 Keyboard Scan Code Set: %#x", KeyboardScanCodeSet);
			break;
		}
		}
	}

	int __fs_kb_Ioctl(struct Inode *, unsigned long, void *)
	{
		return 0;
	}

	const struct InodeOperations KbdOps = {
		.Lookup = nullptr,
		.Create = nullptr,
		.Remove = nullptr,
		.Rename = nullptr,
		.Read = nullptr,
		.Write = nullptr,
		.Truncate = nullptr,
		.Open = nullptr,
		.Close = nullptr,
		.Ioctl = __fs_kb_Ioctl,
		.ReadDir = nullptr,
		.MkDir = nullptr,
		.RmDir = nullptr,
		.SymLink = nullptr,
		.ReadLink = nullptr,
		.Seek = nullptr,
		.Stat = nullptr,
	};

	int InitializeKeyboard()
	{
		// PS2WriteData(PS2_KBD_CMD_RESET);
		// uint8_t test = PS2ReadData();
		// if (test != PS2_KBD_RESP_TEST_PASSED &&
		// 	test != PS2_KBD_RESP_ACK)
		// {
		// 	trace("PS/2 keyboard reset failed (%#x)", test);
		// 	return -EFAULT;
		// }

		PS2WriteData(PS2_KBD_CMD_DEFAULTS);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to set defaults");

		PS2WriteData(PS2_KBD_CMD_SCAN_CODE_SET);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to set scan code set");

		/* We want Scan Code Set 1 */
		PS2WriteData(PS2_KBD_SCAN_CODE_SET_2); /* It will set to 1 but with translation? */
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to set scan code set 2");

		PS2WriteData(PS2_KBD_CMD_SCAN_CODE_SET);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to set scan code set");

		PS2WriteData(PS2_KBD_SCAN_CODE_GET_CURRENT);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to get current scan code set");

		KeyboardScanCodeSet = PS2ReadAfterACK();
		trace("PS/2 Keyboard Scan Code Set: 0x%X", KeyboardScanCodeSet);
		PS2ClearOutputBuffer();

		PS2WriteData(PS2_KBD_CMD_ENABLE_SCANNING);

		v0::RegisterInterruptHandler(DriverID, 1, (void *)PS2KbdInterruptHandler);

		KeyboardDevID = v0::RegisterDevice(DriverID, INPUT_TYPE_KEYBOARD, &KbdOps);
		return 0;
	}

	int FinalizeKeyboard()
	{
		PS2WriteData(PS2_KBD_CMD_DISABLE_SCANNING);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to disable scanning");

		v0::UnregisterDevice(DriverID, KeyboardDevID);
		return 0;
	}

	int DetectPS2Keyboard()
	{
		PS2WriteData(PS2_KBD_CMD_DISABLE_SCANNING);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to disable scanning");

		PS2WriteData(PS2_KBD_CMD_IDENTIFY);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 keyboard failed to identify");

		uint8_t recByte;
		int timeout = 1000000;
		while (timeout--)
		{
			recByte = PS2ReadData();
			if (recByte != PS2_ACK)
				break;
		}
		Device1ID[0] = recByte;

		timeout = 1000000;
		while (timeout--)
		{
			recByte = PS2ReadData();
			if (recByte != PS2_ACK)
				break;
		}
		if (timeout == 0)
			trace("PS/2 keyboard second byte timed out");
		else
			Device1ID[1] = recByte;

		trace("PS2 Keyboard Device: 0x%X 0x%X", Device1ID[0], Device1ID[1]);
		return 0;
	}
}

#endif
