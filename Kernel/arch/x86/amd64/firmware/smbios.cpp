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

#include "smbios.hpp"

#include <memory/virtual.hpp>
#include <boot/binfo.h>
#include <convert.h>
#include <log.hpp>
#include <efi.h>
#include <io.h>

extern struct BootInfo bInfo;

namespace
{
	VOID *SearchSMBIOS(EFI_SYSTEM_TABLE *SystemTable)
	{
		EFI_GUID Smbios3TableGuid = SMBIOS3_TABLE_GUID;
		EFI_GUID SmbiosTableGuid = SMBIOS_TABLE_GUID;

		VOID *Smbios2Ptr = NULL;
		for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++)
		{
			EFI_CONFIGURATION_TABLE *config = &SystemTable->ConfigurationTable[i];

			if (CompareGuid(&config->VendorGuid, &Smbios3TableGuid))
			{
				debug("Found SMBIOS 3.0 at: %#lx", config->VendorTable);
				return config->VendorTable;
			}

			if (CompareGuid(&config->VendorGuid, &SmbiosTableGuid))
			{
				debug("Found SMBIOS at: %#lx", config->VendorTable);
				Smbios2Ptr = config->VendorTable;
			}
		}

		return Smbios2Ptr;
	}
}

void *FindSMBIOS()
{
	if (bInfo.EFI.Info.ST == 1 && Memory::Virtual().Check(bInfo.EFI.SystemTable) && bInfo.EFI.SystemTable != nullptr)
	{
		debug("Searching for SMBIOS in EFI System Table.");
		auto smbios = SearchSMBIOS((EFI_SYSTEM_TABLE *)bInfo.EFI.SystemTable);
		if (smbios)
			return smbios;
	}

	for (fnx::void_t i = 0xF0000; i < 0x100000; i += 0x10)
	{
		if (memcmp(i, "_SM_", 4) == 0 || memcmp(i, "_SM3_", 5) == 0)
		{
			debug("Found SMBIOS at %#lx", i.get());
			return i;
		}
	}

	return nullptr;
}

namespace SMBIOS
{
	bool CheckSMBIOS()
	{
		if (bInfo.SMBIOSPtr != nullptr && bInfo.SMBIOSPtr < (void *)0xFFFFFFFFFFFF0000)
		{
			debug("SMBIOS is available (%#lx).", bInfo.SMBIOSPtr);
			return true;
		}

		debug("SMBIOS is not available. (%#lx)", bInfo.SMBIOSPtr);
		return false;
	}

	SMBIOSEntryPoint *GetSMBIOSEntryPoint() { return (SMBIOSEntryPoint *)bInfo.SMBIOSPtr; }

	__no_sanitize("alignment") static inline int SMBIOSTableLength(SMBIOSHeader *Hdr)
	{
		int i;
		const char *strtab = (char *)Hdr + Hdr->Length;
		for (i = 1; strtab[i - 1] != '\0' || strtab[i] != '\0'; i++)
			;
		return Hdr->Length + i + 1;
	}

	__no_sanitize("alignment") void *GetSMBIOSHeader(SMBIOSType Type)
	{
		if (!CheckSMBIOS())
			return nullptr;

		SMBIOSEntryPoint *Header = (SMBIOSEntryPoint *)bInfo.SMBIOSPtr;
		debug("Getting SMBIOS header for type %d", Type);

		struct SMBIOSHeader *hdr = (SMBIOSHeader *)(uintptr_t)Header->TableAddress;
		for (int i = 0; i <= 11; i++)
		{
			if (hdr < (void *)(uintptr_t)(Header->TableAddress + Header->TableLength))
				if (hdr->Type == Type)
				{
					debug("Found SMBIOS header for type %d at %#lx", Type, hdr);
					return hdr;
				}
			hdr = (struct SMBIOSHeader *)((uintptr_t)hdr + SMBIOSTableLength(hdr));
		}
		return nullptr;
	}

	SMBIOSBIOSInformation *GetBIOSInformation() { return (SMBIOSBIOSInformation *)GetSMBIOSHeader(SMBIOSTypeBIOSInformation); }

	SMBIOSSystemInformation *GetSystemInformation() { return (SMBIOSSystemInformation *)GetSMBIOSHeader(SMBIOSTypeSystemInformation); }

	SMBIOSBaseBoardInformation *GetBaseBoardInformation() { return (SMBIOSBaseBoardInformation *)GetSMBIOSHeader(SMBIOSTypeBaseBoardInformation); }

	SMBIOSProcessorInformation *GetProcessorInformation() { return (SMBIOSProcessorInformation *)GetSMBIOSHeader(SMBIOSTypeProcessorInformation); }

	SMBIOSMemoryArray *GetMemoryArray() { return (SMBIOSMemoryArray *)GetSMBIOSHeader(SMBIOSTypePhysicalMemoryArray); }

	SMBIOSMemoryDevice *GetMemoryDevice() { return (SMBIOSMemoryDevice *)GetSMBIOSHeader(SMBIOSTypeMemoryDevice); }

	SMBIOSMemoryArrayMappedAddress *GetMemoryArrayMappedAddress() { return (SMBIOSMemoryArrayMappedAddress *)GetSMBIOSHeader(SMBIOSTypeMemoryArrayMappedAddress); }

	SMBIOSMemoryDeviceMappedAddress *GetMemoryDeviceMappedAddress() { return (SMBIOSMemoryDeviceMappedAddress *)GetSMBIOSHeader(SMBIOSTypeMemoryDeviceMappedAddress); }
}
