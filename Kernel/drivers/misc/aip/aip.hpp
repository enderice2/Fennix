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

#pragma once

#include <types.h>
#include <cpu.hpp>

namespace Driver::AdvancedIntegratedPeripheral
{
#define PIC1_CMD 0x20
#define PIC1_DATA (PIC1_CMD + 1)
#define PIC2_CMD 0xA0
#define PIC2_DATA (PIC2_CMD + 1)
#define _PIC_EOI 0x20

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD PS2_STATUS
#define PS2_ACK 0xFA
#define PS2_TEST_PASSED 0x55
#define PS2_TEST_FAILED 0xFC

#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_READ_CONFIG_N(n) (PS2_CMD_READ_CONFIG + n)
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_WRITE_CONFIG_N(n) (PS2_CMD_WRITE_CONFIG + n)
#define PS2_CMD_DISABLE_PORT_2 0xA7
#define PS2_CMD_ENABLE_PORT_2 0xA8
#define PS2_CMD_TEST_PORT_2 0xA9
#define PS2_CMD_TEST_CONTROLLER 0xAA
#define PS2_CMD_TEST_PORT_1 0xAB
#define PS2_CMD_DIAGNOSTIC_DUMP 0xAC
#define PS2_CMD_DISABLE_PORT_1 0xAD
#define PS2_CMD_ENABLE_PORT_1 0xAE
#define PS2_CMD_READ_INPUT_PORT 0xC0
#define PS2_CMD_COPY_INPUT_0_3_TO_4_7_STATUS 0xC1
#define PS2_CMD_COPY_INPUT_4_7_TO_4_7_STATUS 0xC2
#define PS2_CMD_READ_OUTPUT_PORT 0xD0
#define PS2_CMD_WRITE_NEXT_BYTE_TO_OUTPUT_PORT 0xD1
#define PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_1_OUTPUT 0xD2
#define PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_OUTPUT 0xD3
#define PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT 0xD4
#define PS2_CMD_PULSE_OUTPUT_LINE(n) (0xF0 + n)

	typedef union
	{
		struct
		{
			uint8_t OutputBufferFull : 1;
			uint8_t InputBufferFull : 1;
			uint8_t SystemFlag : 1;
			uint8_t CommandData : 1;
			uint8_t Unknown1 : 1;
			uint8_t Unknown2 : 1;
			uint8_t TimeoutError : 1;
			uint8_t ParityError : 1;
		};
		uint8_t Raw;
	} PS2_STATUSES;

	typedef union
	{
		struct
		{
			uint8_t Port1Interrupt : 1;
			uint8_t Port2Interrupt : 1;
			uint8_t SystemFlag : 1;
			uint8_t Zero0 : 1;
			uint8_t Port1Clock : 1;
			uint8_t Port2Clock : 1;
			uint8_t Port1Translation : 1;
			uint8_t Zero1 : 1;
		};
		uint8_t Raw;
	} PS2_CONFIGURATION;

	typedef union
	{
		struct
		{
			uint8_t SystemReset : 1;
			uint8_t A20Gate : 1;
			uint8_t Port2Clock : 1;
			uint8_t Port2Data : 1;
			uint8_t Port1OutputBufferFull : 1;
			uint8_t Port2OutputBufferFull : 1;
			uint8_t Port1InputBufferFull : 1;
			uint8_t Port2InputBufferFull : 1;
		};
		uint8_t Raw;
	} PS2_OUTPUT_PORT;

#define WaitOutput PS2Wait(true)
#define WaitInput PS2Wait(false)

#define PS2_KBD_CMD_SET_LEDS 0xED
#define PS2_KBD_CMD_ECHO 0xEE
#define PS2_KBD_CMD_SCAN_CODE_SET 0xF0
#define PS2_KBD_CMD_IDENTIFY 0xF2
#define PS2_KBD_CMD_TYPEMATIC 0xF3
#define PS2_KBD_CMD_ENABLE_SCANNING 0xF4
#define PS2_KBD_CMD_DISABLE_SCANNING 0xF5
#define PS2_KBD_CMD_DEFAULTS 0xF6
#define PS2_KBD_CMD_ALL_TYPEMATIC 0xF7
#define PS2_KBD_CMD_ALL_MAKE_RELEASE 0xF8
#define PS2_KBD_CMD_ALL_MAKE 0xF9
#define PS2_KBD_CMD_ALL_TYPEMATIC_MAKE_RELEASE 0xFA
#define PS2_KBD_CMD_SPECIFIC_TYPEMATIC 0xFB
#define PS2_KBD_CMD_SPECIFIC_MAKE_RELEASE 0xFC
#define PS2_KBD_CMD_SPECIFIC_MAKE 0xFD
#define PS2_KBD_CMD_RESEND 0xFE
#define PS2_KBD_CMD_RESET 0xFF

#define PS2_KBD_RESP_ACK 0xFA
#define PS2_KBD_RESP_ECHO 0xEE
#define PS2_KBD_RESP_RESEND 0xFE
#define PS2_KBD_RESP_TEST_PASSED 0xAA
#define PS2_KBD_RESP_TEST_FAILED 0xFC
#define PS2_KBD_RESP_TEST_FAILED_2 0xFD

	typedef enum
	{
		PS2_KBD_LED_SCROLL_LOCK = 1,
		PS2_KBD_LED_NUM_LOCK = 2,
		PS2_KBD_LED_CAPS_LOCK = 4
	} PS2_KBD_LEDS;

	typedef enum
	{
		PS2_KBD_SCAN_CODE_GET_CURRENT = 0,
		PS2_KBD_SCAN_CODE_SET_1 = 1,
		PS2_KBD_SCAN_CODE_SET_2 = 2,
		PS2_KBD_SCAN_CODE_SET_3 = 3,

		PS2_KBD_SC_SET_1 = 0x43,
		PS2_KBD_SC_SET_2 = 0x41,
		PS2_KBD_SC_SET_3 = 0x3F
	} PS2_KBD_SCAN_CODE_SET;

	typedef union
	{
		struct
		{
			/**
			 * 00000b - 30Hz
			 * 11111b - 2Hz
			 */
			uint8_t RepeatRate : 5;

			/**
			 * 00b - 250ms
			 * 01b - 500ms
			 * 10b - 750ms
			 * 11b - 1000ms
			 */
			uint8_t Delay : 2;

			/**
			 * Must be zero
			 */
			uint8_t Zero : 1;
		};
		uint8_t Raw;
	} PS2_KBD_TYPEMATIC;

#define PS2_MOUSE_CMD_SET_SCALING_1_1 0xE6
#define PS2_MOUSE_CMD_SET_SCALING_2_1 0xE7
#define PS2_MOUSE_CMD_SET_RESOLUTION 0xE8
#define PS2_MOUSE_CMD_GET_STATUS 0xE9
#define PS2_MOUSE_CMD_SET_STREAM_MODE 0xEA
#define PS2_MOUSE_CMD_READ_DATA 0xEB
#define PS2_MOUSE_CMD_RESET_WRAP_MODE 0xEC
#define PS2_MOUSE_CMD_SET_WRAP_MODE 0xEE
#define PS2_MOUSE_CMD_SET_REMOTE_MODE 0xF0
#define PS2_MOUSE_CMD_READ_ID 0xF2
/** Values: 10, 20, 40, 60, 80, 100, 200 */
#define PS2_MOUSE_CMD_SET_SAMPLE_RATE 0xF3
#define PS2_MOUSE_CMD_ENABLE_DATA_REPORTING 0xF4
#define PS2_MOUSE_CMD_DISABLE_DATA_REPORTING 0xF5
#define PS2_MOUSE_CMD_SET_DEFAULTS 0xF6
#define PS2_MOUSE_CMD_RESEND 0xFE
#define PS2_MOUSE_CMD_RESET 0xFF

#define PS2_MOUSE_RESP_ACK 0xFA
#define PS2_MOUSE_RESP_RESEND 0xFE
#define PS2_MOUSE_RESP_TEST_PASSED 0xAA
#define PS2_MOUSE_RESP_TEST_FAILED 0xFC

	typedef enum
	{
		PS2_MOUSE_RES_1 = 0,
		PS2_MOUSE_RES_2 = 1,
		PS2_MOUSE_RES_4 = 2,
		PS2_MOUSE_RES_8 = 3
	} PS2_MOUSE_RESOLUTION;

	typedef struct
	{
		union
		{
			struct
			{
				uint8_t LeftButton : 1;
				uint8_t RightButton : 1;
				uint8_t MiddleButton : 1;
				uint8_t Always1 : 1;
				uint8_t XSign : 1;
				uint8_t YSign : 1;
				uint8_t XOverflow : 1;
				uint8_t YOverflow : 1;
			} __attribute__((packed));
			uint8_t Raw;
		} Base;

		uint8_t XMovement;
		uint8_t YMovement;

		union
		{
			struct
			{
				uint8_t Z : 4;
				uint8_t Button4 : 1;
				uint8_t Button5 : 1;
				uint8_t Always0 : 1;
				uint8_t Always0_2 : 1;
			} __attribute__((packed));
			uint8_t Raw;
		} ZMovement;
	} PS2_MOUSE_PACKET;

	void PIC_EOI(uint8_t IRQ);
	void IRQ_MASK(uint8_t IRQ);
	void IRQ_UNMASK(uint8_t IRQ);

	void PS2Wait(const bool Output);
	void PS2WriteCommand(uint8_t Command);
	void PS2WriteData(uint8_t Data);
	uint8_t PS2ReadData();
	uint8_t PS2ReadStatus();
	uint8_t PS2ReadAfterACK();
	void PS2ClearOutputBuffer();
	int PS2ACKTimeout();

	extern uint8_t Device1ID[];
	extern uint8_t Device2ID[];

	void PS2KbdInterruptHandler(CPU::TrapFrame *);
	int InitializeKeyboard();
	int FinalizeKeyboard();
	int DetectPS2Keyboard();

	void PS2MouseInterruptHandler(CPU::TrapFrame *);
	int InitializeMouse();
	int FinalizeMouse();
	int DetectPS2Mouse();
	int DetectUART();

	void MasterInterruptHandler(CPU::TrapFrame *);
	void SlaveInterruptHandler(CPU::TrapFrame *);
	int InitializeATA();
	int FinalizeATA();
}
