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

namespace Driver::AdvancedIntegratedPeripheral
{
	extern dev_t DriverID;
	dev_t MouseDevID = -1;
	bool PacketReady = false;
	bool FourPackets = false;
	bool MouseButton45 = false;
	uint8_t Cycle = 0;
	PS2_MOUSE_PACKET Packet = {};

	InputReport mir = {};
	void PS2MouseInterruptHandler(CPU::TrapFrame *)
	{
		uint8_t data = PS2ReadData();
		if (data == PS2_MOUSE_RESP_ACK ||
			data == PS2_MOUSE_RESP_RESEND)
			return;

		if (!PacketReady)
		{
			switch (Cycle)
			{
			case 0:
			{
				if ((data & 0b00001000 /* Always 1 */) == 0)
					return;

				Packet.Base.Raw = data;
				Cycle++;
				break;
			}
			case 1:
			{
				Packet.XMovement = data;
				Cycle++;
				break;
			}
			case 2:
			{
				Packet.YMovement = data;
				if (FourPackets)
					Cycle++;
				else
				{
					Cycle = 0;
					PacketReady = true;
				}
				break;
			}
			case 3:
			{
				Packet.ZMovement.Raw = data;
				Cycle = 0;
				PacketReady = true;
				break;
			}
			default:
				break;
			}
			return;
		}

/* https://stackoverflow.com/a/3208376/9352057 */
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)         \
	((byte) & 0x80 ? '1' : '0'),     \
		((byte) & 0x40 ? '1' : '0'), \
		((byte) & 0x20 ? '1' : '0'), \
		((byte) & 0x10 ? '1' : '0'), \
		((byte) & 0x08 ? '1' : '0'), \
		((byte) & 0x04 ? '1' : '0'), \
		((byte) & 0x02 ? '1' : '0'), \
		((byte) & 0x01 ? '1' : '0')

		debug("PS/2 Mouse Packet: [" BYTE_TO_BINARY_PATTERN ":" BYTE_TO_BINARY_PATTERN ":" BYTE_TO_BINARY_PATTERN ":" BYTE_TO_BINARY_PATTERN "] LB:%d RB:%d MB:%d A1:%d XS:%d YS:%d XO:%d YO:%d | X:%03d Y:%03d | Z:%d B4:%d B5:%d A0:%d A0:%d",
			  BYTE_TO_BINARY(Packet.Base.Raw),
			  BYTE_TO_BINARY(Packet.XMovement),
			  BYTE_TO_BINARY(Packet.YMovement),
			  BYTE_TO_BINARY(Packet.ZMovement.Raw),
			  Packet.Base.LeftButton, Packet.Base.RightButton, Packet.Base.MiddleButton,
			  Packet.Base.Always1,
			  Packet.Base.XSign, Packet.Base.YSign,
			  Packet.Base.XOverflow, Packet.Base.YOverflow,
			  Packet.XMovement, Packet.YMovement,
			  Packet.ZMovement.Z, Packet.ZMovement.Button4, Packet.ZMovement.Button5,
			  Packet.ZMovement.Always0, Packet.ZMovement.Always0_2);

		int X, Y;
		X = Packet.XMovement - (Packet.Base.XSign ? 256 : 0);
		Y = Packet.YMovement - (Packet.Base.YSign ? 256 : 0);

		if (Packet.Base.XOverflow)
			X = 0;

		if (Packet.Base.YOverflow)
			Y = 0;

		mir.Type = INPUT_TYPE_MOUSE;
		mir.Device = MouseDevID;
		mir.Mouse.LeftButton = Packet.Base.LeftButton;
		mir.Mouse.RightButton = Packet.Base.RightButton;
		mir.Mouse.MiddleButton = Packet.Base.MiddleButton;
		mir.Mouse.Button4 = Packet.ZMovement.Button4;
		mir.Mouse.Button5 = Packet.ZMovement.Button5;
		mir.Mouse.X = X;
		mir.Mouse.Y = -Y;
		mir.Mouse.Z = Packet.ZMovement.Z;
		v0::ReportInputEvent(DriverID, &mir);
		PacketReady = false;
	}

	void MouseSampleRate(uint8_t SampleRate)
	{
		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
		PS2ReadData();

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(SampleRate);
		PS2ReadData();
	}

	int __fs_ms_Ioctl(struct Inode *, unsigned long, void *)
	{
		return 0;
	}

	const struct InodeOperations MouseOps = {
		.Lookup = nullptr,
		.Create = nullptr,
		.Remove = nullptr,
		.Rename = nullptr,
		.Read = nullptr,
		.Write = nullptr,
		.Truncate = nullptr,
		.Open = nullptr,
		.Close = nullptr,
		.Ioctl = __fs_ms_Ioctl,
		.ReadDir = nullptr,
		.MkDir = nullptr,
		.RmDir = nullptr,
		.SymLink = nullptr,
		.ReadLink = nullptr,
		.Seek = nullptr,
		.Stat = nullptr,
	};

	int InitializeMouse()
	{
		PS2WriteData(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_RESET);
		uint8_t test = PS2ReadData();
		if (test != PS2_MOUSE_RESP_TEST_PASSED &&
			test != PS2_MOUSE_RESP_ACK)
		{
			trace("PS/2 mouse reset failed! (%#x)", test);
			return -EFAULT;
		}

		v0::RegisterInterruptHandler(DriverID, 12, (void *)PS2MouseInterruptHandler);

		MouseDevID = v0::RegisterDevice(DriverID, INPUT_TYPE_MOUSE, &MouseOps);

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_SET_DEFAULTS);
		PS2ReadData();

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_ENABLE_DATA_REPORTING);

		MouseSampleRate(200);
		MouseSampleRate(100);
		MouseSampleRate(80);

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_READ_ID);
		uint8_t Device2ID = PS2ReadData();
		trace("PS/2 Mouse ID: %#x", Device2ID);

		MouseSampleRate(200);
		MouseSampleRate(200);
		MouseSampleRate(80);

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_READ_ID);
		Device2ID = PS2ReadData();
		trace("PS/2 Mouse ID: %#x", Device2ID);

		if (Device2ID >= 3 && Device2ID <= 4)
			FourPackets = true;
		if (Device2ID == 4)
			MouseButton45 = true;

		return 0;
	}

	int FinalizeMouse()
	{
		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_DISABLE_DATA_REPORTING);

		v0::UnregisterDevice(DriverID, MouseDevID);
		return 0;
	}

	int DetectPS2Mouse()
	{
		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_DISABLE_DATA_REPORTING);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 mouse failed to disable data reporting!");

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_READ_ID);
		if (PS2ACKTimeout() != 0)
			trace("PS/2 mouse failed to read ID!");

		uint8_t recByte;
		int timeout = 1000000;
		while (timeout--)
		{
			recByte = PS2ReadData();
			if (recByte != PS2_ACK)
				break;
		}
		Device2ID[0] = recByte;

		timeout = 1000000;
		while (timeout--)
		{
			recByte = PS2ReadData();
			if (recByte != PS2_ACK)
				break;
		}
		Device2ID[1] = recByte;

		trace("PS2 Mouse Device: 0x%X 0x%X", Device2ID[0], Device2ID[1]);
		return 0;
	}
}

#endif
