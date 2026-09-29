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

#ifndef __FENNIX_KERNEL_APIC_H__
#define __FENNIX_KERNEL_APIC_H__

#include <types.h>

#include <irq.hpp>
#include <cpu.hpp>

namespace APIC
{
	enum APICRegisters
	{
		// source from: https://github.com/pdoane/osdev/blob/master/intr/local_apic.c
		APIC_ID = 0x20,		  // Local APIC ID
		APIC_VER = 0x30,	  // Local APIC Version
		APIC_TPR = 0x80,	  // Task Priority
		APIC_APR = 0x90,	  // Arbitration Priority
		APIC_PPR = 0xA0,	  // Processor Priority
		APIC_EOI = 0xB0,	  // EOI
		APIC_RRD = 0xC0,	  // Remote Read
		APIC_LDR = 0xD0,	  // Logical Destination
		APIC_DFR = 0xE0,	  // Destination Format
		APIC_SVR = 0xF0,	  // Spurious Interrupt Vector
		APIC_ISR = 0x100,	  // In-Service (8 registers)
		APIC_TMR = 0x180,	  // Trigger Mode (8 registers)
		APIC_IRR = 0x200,	  // Interrupt Request (8 registers)
		APIC_ESR = 0x280,	  // Error Status
		APIC_ICRLO = 0x300,	  // Interrupt Command
		APIC_ICRHI = 0x310,	  // Interrupt Command [63:32]
		APIC_TIMER = 0x320,	  // LVT Timer
		APIC_THERMAL = 0x330, // LVT Thermal Sensor
		APIC_PERF = 0x340,	  // LVT Performance Counter
		APIC_LINT0 = 0x350,	  // LVT LINT0
		APIC_LINT1 = 0x360,	  // LVT LINT1
		APIC_ERROR = 0x370,	  // LVT Error
		APIC_TICR = 0x380,	  // Initial Count (for Timer)
		APIC_TCCR = 0x390,	  // Current Count (for Timer)
		APIC_TDCR = 0x3E0,	  // Divide Configuration (for Timer)
	};

	enum IOAPICRegisters
	{
		GetIOAPICVersion = 0x1
	};

	enum IOAPICFlags
	{
		ActiveHighLow = 2,
		EdgeLevel = 8
	};

	enum APICDeliveryMode
	{
		Fixed = 0b000,
		LowestPriority = 0b001, /* Reserved */
		SMI = 0b010,
		APIC_DELIVERY_MODE_RESERVED0 = 0b011, /* Reserved */
		NMI = 0b100,
		INIT = 0b101,
		Startup = 0b110,
		ExtINT = 0b111 /* Reserved */
	};

	enum APICDestinationMode
	{
		Physical = 0b0,
		Logical = 0b1
	};

	enum APICDeliveryStatus
	{
		Idle = 0b0,
		SendPending = 0b1
	};

	enum APICLevel
	{
		DeAssert = 0b0,
		Assert = 0b1
	};

	enum APICTriggerMode
	{
		Edge = 0b0,
		Level = 0b1
	};

	enum APICDestinationShorthand
	{
		NoShorthand = 0b00,
		Self = 0b01,
		AllIncludingSelf = 0b10,
		AllExcludingSelf = 0b11
	};

	enum LVTTimerDivide
	{
		DivideBy2 = 0b000,
		DivideBy4 = 0b001,
		DivideBy8 = 0b010,
		DivideBy16 = 0b011,
		DivideBy32 = 0b100,
		DivideBy64 = 0b101,
		DivideBy128 = 0b110,
		DivideBy1 = 0b111
	};

	enum LVTTimerMask
	{
		Unmasked = 0b0,
		Masked = 0b1
	};

	enum LVTTimerMode
	{
		OneShot = 0b00,
		Periodic = 0b01,
		TSCDeadline = 0b10
	};

	typedef union
	{
		struct
		{
			/** @brief Interrupt Vector */
			uint64_t Vector : 8;
			/** @brief Reserved */
			uint64_t Reserved0 : 4;
			/**
			 * @brief Delivery Status
			 *
			 * 0: Idle
			 * 1: Send Pending
			 */
			uint64_t DeliveryStatus : 1;
			/** @brief Reserved */
			uint64_t Reserved1 : 3;
			/**
			 * @brief Mask
			 *
			 * 0: Not masked
			 * 1: Masked
			 */
			uint64_t Mask : 1;
			/** @brief Timer Mode
			 *
			 * 0: One-shot
			 * 1: Periodic
			 * 2: TSC-Deadline
			 */
			uint64_t TimerMode : 1;
			/** @brief Reserved */
			uint64_t Reserved2 : 14;
		};
		uint64_t raw;
	} __packed LVTTimer;

	typedef union
	{
		struct
		{
			/** @brief Spurious Vector */
			uint64_t Vector : 8;
			/** @brief Enable or disable APIC software */
			uint64_t Software : 1;
			/** @brief Focus Processor Checking */
			uint64_t FocusProcessorChecking : 1;
			/** @brief Reserved */
			uint64_t Reserved : 2;
			/** @brief Disable EOI Broadcast */
			uint64_t DisableEOIBroadcast : 1;
			/** @brief Reserved */
			uint64_t Reserved1 : 19;
		};
		uint64_t raw;
	} __packed Spurious;

	typedef union
	{
		struct
		{
			/** @brief Interrupt Vector */
			uint64_t Vector : 8;
			/** @brief Delivery Mode */
			uint64_t DeliveryMode : 3;
			/** @brief Destination Mode
			 *
			 * 0: Physical
			 * 1: Logical
			 */
			uint64_t DestinationMode : 1;
			/** @brief Delivery Status
			 *
			 * @note Reserved when in x2APIC mode
			 */
			uint64_t DeliveryStatus : 1;
			/** @brief Reserved */
			uint64_t Reserved0 : 1;
			/** @brief Level
			 *
			 * 0: Deassert
			 * 1: Assert
			 */
			uint64_t Level : 1;
			/** @brief Trigger Mode
			 *
			 * 0: Edge
			 * 1: Level
			 */
			uint64_t TriggerMode : 1;
			/** @brief Reserved */
			uint64_t Reserved1 : 2;
			/** @brief Destination Shorthand
			 *
			 * 0: No shorthand
			 * 1: Self
			 * 2: All including self
			 * 3: All excluding self
			 */
			uint64_t DestinationShorthand : 2;
			/** @brief Reserved */
			uint64_t Reserved2 : 12;
		};
		uint64_t raw;
	} __packed InterruptCommandRegisterLow;

	typedef union
	{
		struct
		{
			/** @brief Reserved */
			uint64_t Reserved0 : 24;
			/** @brief Destination */
			uint64_t Destination : 8;
		};
		uint64_t raw;
	} __packed InterruptCommandRegisterHigh;

	typedef union
	{
		struct
		{
			/** @brief Interrupt Vector */
			uint64_t Vector : 8;
			/** @brief Delivery Mode */
			uint64_t DeliveryMode : 3;
			/** @brief Destination Mode
			 *
			 * 0: Physical
			 * 1: Logical
			 */
			uint64_t DestinationMode : 1;
			/** @brief Delivery Status */
			uint64_t DeliveryStatus : 1;
			/** @brief Interrupt Input Pin Polarity
			 *
			 * 0: Active High
			 * 1: Active Low
			 */
			uint64_t Polarity : 1;
			/** @brief Remote IRR */
			uint64_t RemoteIRR : 1;
			/** @brief Trigger Mode
			 *
			 * 0: Edge
			 * 1: Level
			 */
			uint64_t TriggerMode : 1;
			/** @brief Mask */
			uint64_t Mask : 1;
			/** @brief Reserved */
			uint64_t Reserved0 : 15;
			/** @brief Reserved */
			uint64_t Reserved1 : 24;
			/** @brief Destination */
			uint64_t DestinationID : 8;
		};
		struct
		{
			uint64_t Low;
			uint64_t High;
		} split;
		uint64_t raw;
	} __packed IOAPICRedirectEntry;

	typedef union
	{
		struct
		{
			uint64_t Version : 8;
			uint64_t Reserved : 8;
			uint64_t MaximumRedirectionEntry : 8;
			uint64_t Reserved2 : 8;
		};
		uint64_t raw;
	} __packed IOAPICVersion;

	class APIC
	{
	private:
		bool x2APICSupported = false;
		uint64_t APICBaseAddress = 0;

	public:
		decltype(x2APICSupported) &x2APIC = x2APICSupported;

		uint32_t Read(uint32_t Register);
		void Write(uint32_t Register, uint32_t Value);
		void IOWrite(uint64_t Base, uint32_t Register, uint32_t Value);
		uint32_t IORead(uint64_t Base, uint32_t Register);
		void EOI();
		void RedirectIRQs(int CPU = 0);
		void WaitForIPI();
		void IPI(uint8_t CPU, InterruptCommandRegisterLow icr);
		void SendInitIPI(uint8_t CPU);
		void SendStartupIPI(uint8_t CPU, uint64_t StartupAddress);
		uint32_t IOGetMaxRedirect(uint32_t APICID);
		void RawRedirectIRQ(uint16_t Vector, uint32_t GSI, uint16_t Flags, int CPU, int Status);
		void RedirectIRQ(int CPU, uint16_t IRQ, int Status);
		APIC(int Core);
		~APIC();
	};

	class Timer : public Interrupt::Handler
	{
	private:
		APIC *lapic;
		uint64_t Ticks = 0;
		int OnInterruptReceived(CPU::TrapFrame *Frame);

	public:
		uint64_t GetTicks() { return Ticks; }
		void OneShot(uint32_t Vector, uint64_t Miliseconds);
		Timer(APIC *apic);
		~Timer();
	};
}

#endif // !__FENNIX_KERNEL_APIC_H__
