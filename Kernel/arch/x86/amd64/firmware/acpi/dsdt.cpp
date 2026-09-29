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

#include <acpi.hpp>

#include <cpu/apic.hpp>
#include <time.hpp>
#include <debug.h>
#include <smp.hpp>
#include <io.h>

#include "../kernel.h"

#define ACPI_TIMER 0x0001
#define ACPI_BUSMASTER 0x0010
#define ACPI_GLOBAL 0x0020
#define ACPI_POWER_BUTTON 0x0100
#define ACPI_SLEEP_BUTTON 0x0200
#define ACPI_RTC_ALARM 0x0400
#define ACPI_PCIE_WAKE 0x4000
#define ACPI_WAKE 0x8000

extern std::atomic<bool> ExceptionLock;

namespace Platform
{
	__always_inline inline bool IsCanonical(uint64_t Address)
	{
		return ((Address <= 0x00007FFFFFFFFFFF) ||
				((Address >= 0xFFFF800000000000) &&
				 (Address <= 0xFFFFFFFFFFFFFFFF)));
	}

#define ACPI_ENABLED 0x0001
#define ACPI_SLEEP 0x2000

#define ACPI_GAS_MMIO 0
#define ACPI_GAS_IO 1
#define ACPI_GAS_PCI 2

	int DSDT::OnInterruptReceived(CPU::TrapFrame *)
	{
		debug("SCI Handle Triggered");
		uint16_t Event = 0;
		{
#if defined(__amd64__) || defined(__i386__)
			uint16_t a = 0, b = 0;
			if (acpi->FADT->PM1aEventBlock)
			{
				a = inw(static_cast<uint16_t>(acpi->FADT->PM1aEventBlock));
				outw(static_cast<uint16_t>(acpi->FADT->PM1aEventBlock), a);
			}
			if (acpi->FADT->PM1bEventBlock)
			{
				b = inw(static_cast<uint16_t>(acpi->FADT->PM1bEventBlock));
				outw(static_cast<uint16_t>(acpi->FADT->PM1bEventBlock), b);
			}
			Event = a | b;
#endif
		}

#ifdef DEBUG
		char dbgEvStr[128];
		dbgEvStr[0] = '\0';
		if (Event & ACPI_BUSMASTER)
			strcat(dbgEvStr, "BUSMASTER ");
		if (Event & ACPI_GLOBAL)
			strcat(dbgEvStr, "GLOBAL ");
		if (Event & ACPI_POWER_BUTTON)
			strcat(dbgEvStr, "POWER_BUTTON ");
		if (Event & ACPI_SLEEP_BUTTON)
			strcat(dbgEvStr, "SLEEP_BUTTON ");
		if (Event & ACPI_RTC_ALARM)
			strcat(dbgEvStr, "RTC_ALARM ");
		if (Event & ACPI_PCIE_WAKE)
			strcat(dbgEvStr, "PCIE_WAKE ");
		if (Event & ACPI_WAKE)
			strcat(dbgEvStr, "WAKE ");
		if (Event & ACPI_TIMER)
			strcat(dbgEvStr, "ACPI_TIMER ");
		klog("SCI Event: %s", dbgEvStr);
#endif
		if (Event & ACPI_BUSMASTER)
		{
			fixme("ACPI Busmaster");
		}
		else if (Event & ACPI_GLOBAL)
		{
			fixme("ACPI Global");
		}
		else if (Event & ACPI_POWER_BUTTON)
		{
			if (ExceptionLock.load())
			{
				this->Shutdown();
				CPU::Stop();
			}

			PowerManager->Shutdown();
		}
		else if (Event & ACPI_SLEEP_BUTTON)
		{
			fixme("ACPI Sleep Button");
		}
		else if (Event & ACPI_RTC_ALARM)
		{
			fixme("ACPI RTC Alarm");
		}
		else if (Event & ACPI_PCIE_WAKE)
		{
			fixme("ACPI PCIe Wake");
		}
		else if (Event & ACPI_WAKE)
		{
			fixme("ACPI Wake");
		}
		else if (Event & ACPI_TIMER)
		{
			fixme("ACPI Timer");
		}
		else
		{
			error("ACPI unknown event %#lx on CPU %d",
				  Event, GetCurrentCPU()->ID);
			klog("ACPI unknown event %#lx on CPU %d",
				 Event, GetCurrentCPU()->ID);
		}

		return EOK;
	}

	void DSDT::Shutdown()
	{
		trace("Shutting down...");
		if (SCI_EN != 1)
		{
			error("ACPI Shutdown not supported");
			return;
		}

#if defined(__amd64__) || defined(__i386__)
		if (inw(static_cast<uint16_t>(PM1a_CNT) & SCI_EN) == 0)
		{
			klog("ACPI was disabled, enabling...");
			if (SMI_CMD == 0 || ACPI_ENABLE == 0)
			{
				error("ACPI Shutdown not supported");
				klog("ACPI Shutdown not supported");
				return;
			}

			outb(static_cast<uint16_t>(SMI_CMD), ACPI_ENABLE);

			uint16_t Timeout = 3000;
			while ((inw(static_cast<uint16_t>(PM1a_CNT)) & SCI_EN) == 0 && Timeout-- > 0)
				;

			if (Timeout == 0)
			{
				error("ACPI Shutdown not supported");
				klog("ACPI Shutdown not supported");
				return;
			}

			if (PM1b_CNT)
			{
				Timeout = 3000;
				while ((inw(static_cast<uint16_t>(PM1b_CNT)) & SCI_EN) == 0 && Timeout-- > 0)
					;
			}
		}

		outw(static_cast<uint16_t>(PM1a_CNT), SLP_TYPa | SLP_EN);
		if (PM1b_CNT)
			outw(static_cast<uint16_t>(PM1b_CNT), SLP_TYPb | SLP_EN);
#elif defined(__aarch64__)
		warn("ACPI Shutdown not supported");
#endif
	}

	void DSDT::Reboot()
	{
		trace("Rebooting...");
		switch (acpi->FADT->ResetReg.AddressSpace)
		{
		case ACPI_GAS_MMIO:
		{
			*(uint8_t *)(acpi->FADT->ResetReg.Address) = acpi->FADT->ResetValue;
			break;
		}
		case ACPI_GAS_IO:
		{
#if defined(__amd64__) || defined(__i386__)
			outb(static_cast<uint16_t>(acpi->FADT->ResetReg.Address), acpi->FADT->ResetValue);
#elif defined(__aarch64__)
			warn("ACPI_GAS_IO not supported");
#endif
			break;
		}
		case ACPI_GAS_PCI:
		{
			fixme("ACPI_GAS_PCI not supported.");
			/*
				seg      - 0
				bus      - 0
				dev      - (FADT->ResetReg.Address >> 32) & 0xFFFF
				function - (FADT->ResetReg.Address >> 16) & 0xFFFF
				offset   - FADT->ResetReg.Address & 0xFFFF
				value    - FADT->ResetValue
			*/
			break;
		}
		default:
		{
			error("Unknown reset register address space: %d", acpi->FADT->ResetReg.AddressSpace);
			break;
		}
		}
	}

	DSDT::DSDT(ACPI *acpi) : Interrupt::Handler(acpi->FADT->SCI_Interrupt)
	{
		/* TODO: AML Interpreter */

		this->acpi = acpi;
		uint64_t Address = ((IsCanonical(acpi->FADT->X_Dsdt) && acpi->XSDTSupported) ? acpi->FADT->X_Dsdt : acpi->FADT->Dsdt);
		uint8_t *S5Address = (uint8_t *)(Address) + 36;
		ACPIHeader *Header = (ACPIHeader *)Address;
		Memory::Virtual vmm;
		if (!vmm.Check(Header))
		{
			warn("DSDT is not mapped");
			debug("DSDT: %#lx", Address);
			vmm.SingleMap(Header, Header, Memory::RW);
		}

		size_t Length = Header->Length;
		vmm.Map(Header, Header, Length, Memory::RW);

		while (Length-- > 0)
		{
			if (!memcmp(S5Address, "_S5_", 4))
				break;
			S5Address++;
		}

		if (Length <= 0)
		{
			warn("_S5_ not present in ACPI");
			return;
		}

		if ((*(S5Address - 1) == 0x08 ||
			 (*(S5Address - 2) == 0x08 &&
			  *(S5Address - 1) == '\\')) &&
			*(S5Address + 4) == 0x12)
		{
			S5Address += 5;
			S5Address += ((*S5Address & 0xC0) >> 6) + 2;

			if (*S5Address == 0x0A)
				S5Address++;

			SLP_TYPa = static_cast<uint16_t>(*(S5Address) << 10);
			S5Address++;

			if (*S5Address == 0x0A)
				S5Address++;

			SLP_TYPb = static_cast<uint16_t>(*(S5Address) << 10);
			SMI_CMD = acpi->FADT->SMI_CommandPort;
			ACPI_ENABLE = acpi->FADT->AcpiEnable;
			ACPI_DISABLE = acpi->FADT->AcpiDisable;
			PM1a_CNT = acpi->FADT->PM1aControlBlock;
			PM1b_CNT = acpi->FADT->PM1bControlBlock;
			PM1_CNT_LEN = acpi->FADT->PM1ControlLength;
			SLP_EN = 1 << 13;
			SCI_EN = 1;
			klog("ACPI Shutdown is supported");
			ACPIShutdownSupported = true;

#if defined(__amd64__) || defined(__i386__)
			{
				const uint16_t value = /*ACPI_TIMER |*/ ACPI_BUSMASTER | ACPI_GLOBAL |
									   ACPI_POWER_BUTTON | ACPI_SLEEP_BUTTON | ACPI_RTC_ALARM |
									   ACPI_PCIE_WAKE | ACPI_WAKE;
				uint16_t a = static_cast<uint16_t>(acpi->FADT->PM1aEventBlock + (acpi->FADT->PM1EventLength / 2));
				uint16_t b = static_cast<uint16_t>(acpi->FADT->PM1bEventBlock + (acpi->FADT->PM1EventLength / 2));
				debug("SCI Event: %#x [a:%#x b:%#x]", value, a, b);
				if (acpi->FADT->PM1aEventBlock)
					outw(a, value);
				if (acpi->FADT->PM1bEventBlock)
					outw(b, value);
			}

			{
				uint16_t a = 0, b = 0;
				if (acpi->FADT->PM1aEventBlock)
				{
					a = inw(static_cast<uint16_t>(acpi->FADT->PM1aEventBlock));
					outw(static_cast<uint16_t>(acpi->FADT->PM1aEventBlock), a);
				}
				if (acpi->FADT->PM1bEventBlock)
				{
					b = inw(static_cast<uint16_t>(acpi->FADT->PM1bEventBlock));
					outw(static_cast<uint16_t>(acpi->FADT->PM1bEventBlock), b);
				}
			}

			// ((APIC::APIC *)Interrupts::apic[0])->RedirectIRQ(0, uint8_t(acpi->FADT->SCI_Interrupt), 1);
#endif
			return;
		}
		warn("Failed to parse _S5_ in ACPI");
		SCI_EN = 0;
	}
}
