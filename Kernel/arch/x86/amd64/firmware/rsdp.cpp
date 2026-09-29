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

#include <memory/virtual.hpp>
#include <boot/binfo.h>
#include <convert.h>
#include <log.hpp>
#include <efi.h>
#include <io.h>

extern struct BootInfo bInfo;

namespace
{
	VOID *SearchRSDP(EFI_SYSTEM_TABLE *SystemTable)
	{
		EFI_GUID AcpiTableGuid = EFI_ACPI_TABLE_GUID;
		for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++)
		{
			EFI_CONFIGURATION_TABLE *config = &SystemTable->ConfigurationTable[i];
			if (CompareGuid(&config->VendorGuid, &AcpiTableGuid))
			{
				debug("RSDP found at: %#lx", config->VendorTable);
				return config->VendorTable;
			}
		}

		return NULL;
	}
}

BootInfo::RSDPInfo *FindRSDP()
{
	if (bInfo.EFI.Info.ST == 1 && Memory::Virtual().Check(bInfo.EFI.SystemTable) && bInfo.EFI.SystemTable != nullptr)
	{
		debug("Searching for RSDP in EFI System Table.");

		auto rsdp = SearchRSDP((EFI_SYSTEM_TABLE *)bInfo.EFI.SystemTable);
		if (rsdp)
			return (BootInfo::RSDPInfo *)rsdp;
	}

	/* FIXME: Not always shifting by 4 will work. */
	fnx::void_t EBDABase = mminw((void *)0x40E) << 4;
	BootInfo::RSDPInfo *rsdp;

	for (fnx::void_t ptr = EBDABase;
		 ptr < 0x100000; /* 1MB */
		 ptr += 0x10)
	{
		if (unlikely(ptr == EBDABase + 0x400))
		{
			debug("EBDA is full. Trying to find RSDP in the BIOS area.");
			break;
		}

		rsdp = ptr;
		if (memcmp(rsdp->Signature, "RSD PTR ", 8) == 0)
		{
			debug("Found RSDP at %#lx", rsdp);
			return rsdp;
		}
	}

	for (fnx::void_t ptr = 0xE0000;
		 ptr < 0x100000; /* 1MB */
		 ptr += 0x10)
	{
		rsdp = ptr;
		if (memcmp(rsdp->Signature, "RSD PTR ", 8) == 0)
		{
			debug("Found RSDP at %#lx", rsdp);
			return rsdp;
		}
	}

	return nullptr;
}
