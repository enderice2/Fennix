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

#include <driver.hpp>
#include <cpu.hpp>
#include <pci.hpp>
#include <log.hpp>

#include "aip.hpp"

extern Driver::Manager *DriverManager;
extern PCI::Manager *PCIManager;
namespace Driver::AdvancedIntegratedPeripheral
{
	dev_t DriverID;

	void PIC_EOI(uint8_t IRQ)
	{
		if (IRQ >= 8)
			outb(PIC2_CMD, _PIC_EOI);
		outb(PIC1_CMD, _PIC_EOI);
	}

	void IRQ_MASK(uint8_t IRQ)
	{
		uint16_t port;
		uint8_t value;

		if (IRQ < 8)
			port = PIC1_DATA;
		else
		{
			port = PIC2_DATA;
			IRQ -= 8;
		}

		value = inb(port) | (1 << IRQ);
		outb(port, value);
	}

	void IRQ_UNMASK(uint8_t IRQ)
	{
		uint16_t port;
		uint8_t value;

		if (IRQ < 8)
			port = PIC1_DATA;
		else
		{
			port = PIC2_DATA;
			IRQ -= 8;
		}

		value = inb(port) & ~(1 << IRQ);
		outb(port, value);
	}

	void PS2Wait(const bool Output)
	{
		int Timeout = 100000;
		while (Timeout--)
		{
			PS2_STATUSES Status = {.Raw = inb(PS2_STATUS)};

			if (Output)
			{
				if (Status.OutputBufferFull)
					return;
			}
			else
			{
				if (!Status.InputBufferFull)
					return;
			}
		}

		PS2_STATUSES Status = {.Raw = inb(PS2_STATUS)};
		warn("PS/2 controller timeout! (Status: %#x, %d)", Status.Raw, Timeout);
	}

	void PS2WriteCommand(uint8_t Command)
	{
		WaitInput;
		outb(PS2_CMD, Command);
	}

	void PS2WriteData(uint8_t Data)
	{
		WaitInput;
		outb(PS2_DATA, Data);
	}

	uint8_t PS2ReadData()
	{
		WaitOutput;
		return inb(PS2_DATA);
	}

	uint8_t PS2ReadStatus()
	{
		return inb(PS2_STATUS);
	}

	uint8_t PS2ReadAfterACK()
	{
		uint8_t ret = PS2ReadData();
		while (ret == PS2_ACK)
		{
			WaitOutput;
			ret = inb(PS2_DATA);
		}
		return ret;
	}

	void PS2ClearOutputBuffer()
	{
		PS2_STATUSES Status;
		int timeout = 0x500;
		while (timeout--)
		{
			Status.Raw = inb(PS2_STATUS);
			if (Status.OutputBufferFull == 0)
				return;
			inb(PS2_DATA);
		}
	}

	int PS2ACKTimeout()
	{
		int timeout = 0x500;
		while (timeout > 0)
		{
			if (PS2ReadData() == PS2_ACK)
				return 0;
			timeout--;
		}
		return -ETIMEDOUT;
	}

	bool IsATAPresent()
	{
		outb(0x1F0 + 2, 0);
		outb(0x1F0 + 3, 0);
		outb(0x1F0 + 4, 0);
		outb(0x1F0 + 5, 0);
		outb(0x1F0 + 7, 0xEC);
		if (inb(0x1F0 + 7) == 0 || inb(0x1F0 + 1) != 0)
			return false;
		return true;
	}

	bool IsKeyboard(uint8_t ID)
	{
		/* Common keyboard IDs */
		return ID == 0xAB || ID == 0xAC || ID == 0x5D ||
			   ID == 0x2B || ID == 0x47 || ID == 0x60;
	}

	bool IsMouse(uint8_t ID)
	{
		/* Common mouse IDs */
		return ID == 0x00 || ID == 0x03 || ID == 0x04;
	}

	const char *GetPS2DeviceName(uint8_t ID, uint8_t SubID)
	{
		switch (ID)
		{
		case 0x00:
			return "Standard PS/2 Mouse";
		case 0x03:
			return "Mouse with scroll wheel";
		case 0x04:
			return "Mouse 5 buttons";
		case 0xAB:
		{
			switch (SubID)
			{
			case 0x83: /* Normal */
			case 0x41: /* Translated */
			case 0xC1: /* Normal + Translated */
				return "Standard PS/2 Keyboard";
			case 0x84:
			case 0x54:
				return "IBM Thinkpad/Spacesaver Keyboard";
			case 0x85:
				return "NCD N-97/122-Key Host Connect(ed) Keyboard";
			case 0x86:
				return "122-Key Keyboard";
			case 0x90:
				return "Japanese \"G\" Keyboard";
			case 0x91:
				return "Japanese \"P\" Keyboard";
			case 0x92:
				return "Japanese \"A\" Keyboard";
			default:
				return "Unknown PS/2 Keyboard";
			}
		}
		case 0xAC:
		{
			switch (SubID)
			{
			case 0xA1:
				return "NCD Sun Keyboard";
			default:
				return "Unknown NCD Sun Keyboard";
			}
		}
		case 0x5D:
		case 0x2B:
			return "Trust Keyboard";
		case 0x47:
		case 0x60:
			return "NMB SGI Keyboard";
		default:
			return "Unknown PS/2 Device";
		}
	}

	uint8_t Device1ID[2] = {0x00, 0x00};
	uint8_t Device2ID[2] = {0x00, 0x00};
	bool DualChannel = false;
	bool ATAPresent = false;

	int Entry()
	{
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_1);
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_2);
		PS2ClearOutputBuffer();

		PS2WriteCommand(PS2_CMD_READ_CONFIG);
		PS2_CONFIGURATION cfg = {.Raw = PS2ReadData()};

		DualChannel = cfg.Port2Clock;
		if (DualChannel)
			trace("Dual channel PS/2 controller detected");
		cfg.Port1Interrupt = 1;
		cfg.Port2Interrupt = 1;
		cfg.Port1Translation = 1;

		PS2WriteCommand(PS2_CMD_WRITE_CONFIG);
		PS2WriteData(cfg.Raw);

		PS2WriteCommand(PS2_CMD_TEST_CONTROLLER);
		uint8_t test = PS2ReadData();
		if (test != PS2_TEST_PASSED)
		{
			trace("PS/2 controller self test failed (%#x)", test);
			return -EFAULT;
		}

		PS2WriteCommand(PS2_CMD_WRITE_CONFIG);
		PS2WriteData(cfg.Raw);

		// bool port2avail = false;
		// if (DualChannel)
		// {
		// 	PS2WriteCommand(PS2_CMD_ENABLE_PORT_1);
		// 	PS2WriteCommand(PS2_CMD_READ_CONFIG);
		// 	cfg.Raw = PS2ReadData();
		// 	port2avail = cfg.Port2Clock;
		// 	PS2WriteCommand(PS2_CMD_DISABLE_PORT_1);
		// }

		PS2WriteCommand(PS2_CMD_TEST_PORT_1);
		test = PS2ReadData();
		if (test != 0x00)
		{
			trace("PS/2 Port 1 self test failed (%#x)", test);
			return -EFAULT;
		}

		if (DualChannel)
		{
			PS2WriteCommand(PS2_CMD_TEST_PORT_2);
			test = PS2ReadData();
			if (test != 0x00)
			{
				trace("PS/2 Port 2 self test failed (%#x)", test);
				return -EFAULT;
			}
		}

		PS2WriteCommand(PS2_CMD_ENABLE_PORT_1);
		if (DualChannel)
			PS2WriteCommand(PS2_CMD_ENABLE_PORT_2);

		int errK = InitializeKeyboard();

		int errM = 0;
		if (DualChannel)
			errM = InitializeMouse();

		ATAPresent = IsATAPresent();

		if (errK != 0 && errM != 0 && ATAPresent == false)
			return -ENODEV;
		return 0;
	}

	int Final()
	{
		FinalizeKeyboard();
		FinalizeMouse();
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_1);
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_2);
		return 0;
	}

	int Panic()
	{
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_1);
		PS2WriteCommand(PS2_CMD_DISABLE_PORT_2);
		return 0;
	}

	void __intStub() {}
	int Probe()
	{
		v0::RegisterInterruptHandler(DriverID, 1, (void *)__intStub);
		v0::RegisterInterruptHandler(DriverID, 12, (void *)__intStub);

		int kbd = DetectPS2Keyboard();
		int mouse = DetectPS2Mouse();
		int uart = DetectUART();

		v0::UnregisterAllInterruptHandlers(DriverID, (void *)__intStub);

		if (kbd != 0 && mouse != 0 && uart != 0)
			return -ENODEV;

		if (kbd == 0)
		{
			if (!IsKeyboard(Device1ID[0]))
			{
				trace("PS/2 Port 1 is not a keyboard");
				// return -EINVAL;
			}
		}

		if (mouse == 0)
		{
			if (!IsMouse(Device2ID[0]))
			{
				trace("PS/2 Port 2 is not a mouse");
				// return -EINVAL;
			}
		}

		klog("PS/2 Port 1: %s (0x%X 0x%X)",
			 GetPS2DeviceName(Device1ID[0], Device1ID[1]),
			 Device1ID[0], Device1ID[1]);
		klog("PS/2 Port 2: %s (0x%X 0x%X)",
			 GetPS2DeviceName(Device2ID[0], Device2ID[1]),
			 Device2ID[0], Device2ID[1]);
		return 0;
	}

	REGISTER_BUILTIN_DRIVER(aip,
							"Advanced Integrated Peripheral Driver",
							"enderice2",
							1, 0, 0,
							Entry,
							Final,
							Panic,
							Probe);
}

#endif
