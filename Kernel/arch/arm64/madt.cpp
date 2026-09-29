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

#include "acpi.hpp"

#include <memory.hpp>
#include <debug.h>

#include "../../kernel.h"

namespace ACPI
{
	MADT::MADT(ACPI::MADTHeader *madt)
	{
		trace("Initializing MADT");
		if (!madt)
		{
			error("MADT is NULL");
			return;
		}

		CPUCores = 0;
		LAPICAddress = (LAPIC *)(uintptr_t)madt->LocalControllerAddress;
		for (uint8_t *ptr = (uint8_t *)(madt->Entries);
			 (uintptr_t)(ptr) < (uintptr_t)(madt) + madt->Header.Length;
			 ptr += *(ptr + 1))
		{
			switch (*(ptr))
			{
			case 0:
			{
				if (ptr[4] & 1)
				{
					lapic.push_back((LocalAPIC *)ptr);
					klog("Local APIC %d (APIC %d) found.", lapic.back()->ACPIProcessorId, lapic.back()->APICId);
					CPUCores++;
				}
				break;
			}
			case 1:
			{
				ioapic.push_back((MADTIOApic *)ptr);
				klog("I/O APIC %d (Address %#lx) found.", ioapic.back()->APICID, ioapic.back()->Address);
				Memory::Virtual(KernelPageTable).Map((void *)(uintptr_t)ioapic.back()->Address, (void *)(uintptr_t)ioapic.back()->Address, Memory::PTFlag::RW | Memory::PTFlag::PCD); // Make sure that the address is mapped.
				break;
			}
			case 2:
			{
				iso.push_back((MADTIso *)ptr);
				klog("ISO (IRQ:%#lx, BUS:%#lx, GSI:%#lx, %s/%s) found.",
					 iso.back()->IRQSource, iso.back()->BuSSource, iso.back()->GSI,
					 iso.back()->Flags & 0x00000004 ? "Active High" : "Active Low",
					 iso.back()->Flags & 0x00000100 ? "Edge Triggered" : "Level Triggered");
				break;
			}
			case 4:
			{
				nmi.push_back((MADTNmi *)ptr);
				klog("NMI %#lx (lint:%#lx) found.", nmi.back()->processor, nmi.back()->lint);
				break;
			}
			case 5:
			{
				LAPICAddress = (LAPIC *)ptr;
				klog("APIC found at %#lx", LAPICAddress);
				break;
			}
			default:
			{
				klog("Unknown MADT entry %#lx", *(ptr));
				break;
			}
			}
			Memory::Virtual(KernelPageTable).Map((void *)LAPICAddress, (void *)LAPICAddress, Memory::PTFlag::RW | Memory::PTFlag::PCD); // I should map more than one page?
		}
		CPUCores--; // We start at 0 (BSP) and end at 11 (APs), so we have 12 cores.
		klog("Total CPU cores: %d", CPUCores + 1);
	}

	MADT::~MADT()
	{
	}
}
