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

#include <efi.h>

#include <boot/binfo.h>
#include <debug.h>
#include <memory/virtual.hpp>

extern struct BootInfo bInfo;

namespace
{
	VOID SearchSMBIOS(EFI_SYSTEM_TABLE *SystemTable)
	{
		EFI_GUID Smbios3TableGuid = SMBIOS3_TABLE_GUID;
		EFI_GUID SmbiosTableGuid = SMBIOS_TABLE_GUID;

		for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++)
		{
			EFI_CONFIGURATION_TABLE *config = &SystemTable->ConfigurationTable[i];

			/* Does this device have multiple SMBIOS tables? If so, prefer SMBIOS 3.0 */

			if (CompareGuid(&config->VendorGuid, &Smbios3TableGuid))
			{
				debug("SMBIOS3 found at address: %#lx", config->VendorTable);
				bInfo.SMBIOSPtr = config->VendorTable;
				return;
			}

			if (CompareGuid(&config->VendorGuid, &SmbiosTableGuid))
			{
				debug("SMBIOS found at address: %#lx", config->VendorTable);
				bInfo.SMBIOSPtr = config->VendorTable;
			}
		}
	}

	VOID SearchRSDP(EFI_SYSTEM_TABLE *SystemTable)
	{
		EFI_GUID AcpiTableGuid = EFI_ACPI_TABLE_GUID;
		for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++)
		{
			EFI_CONFIGURATION_TABLE *config = &SystemTable->ConfigurationTable[i];
			if (CompareGuid(&config->VendorGuid, &AcpiTableGuid))
			{
				bInfo.RSDP = (BootInfo::RSDPInfo *)config->VendorTable;
				debug("RSDP found at address: %#lx", bInfo.RSDP);
				break;
			}
		}
	}

	VOID InitializeMemoryEntries(EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN NumberOfEntries, UINTN DescriptorSize)
	{
		debug("Memory map: %#lx", MemoryMap);
		debug("Number of entries: %d", NumberOfEntries);
		debug("Descriptor size: %d", DescriptorSize);

		for (UINTN i = 0; i < NumberOfEntries; i++)
		{
			EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + i * DescriptorSize);
			UNUSED(desc);

			// #ifdef DEBUG
			// 		const char *EFI_MEMORY_TYPE_STRINGS[] = {
			// 			"ReservedMemoryType",
			// 			"LoaderCode",
			// 			"LoaderData",
			// 			"BootServicesCode",
			// 			"BootServicesData",
			// 			"RuntimeServicesCode",
			// 			"RuntimeServicesData",
			// 			"ConventionalMemory",
			// 			"UnusableMemory",
			// 			"ACPIReclaimMemory",
			// 			"ACPIMemoryNVS",
			// 			"MemoryMappedIO",
			// 			"MemoryMappedIOPortSpace",
			// 			"PalCode",
			// 			"PersistentMemory",
			// 			"MaxMemoryType",
			// 			"out of bounds?!"};
			// 		size_t type = desc->Type;
			// 		if (type > sizeof(EFI_MEMORY_TYPE_STRINGS) / sizeof(EFI_MEMORY_TYPE_STRINGS[0]))
			// 		{
			// 			type = 16;
			// 			debug("oh uh, %d is out of bounds!!! t:%#lx p:%#lx v:%#lx n:%lu a:%lx",
			// 				  i, desc->Type,
			// 				  desc->PhysicalStart,
			// 				  desc->VirtualStart,
			// 				  desc->NumberOfPages,
			// 				  desc->Attribute);
			// 		}
			// 		debug("Entry %d: Type: %s, PhysicalStart: %p, VirtualStart: %p, NumberOfPages: %lu, Attribute: %lx",
			// 			  i, EFI_MEMORY_TYPE_STRINGS[type],
			// 			  desc->PhysicalStart,
			// 			  desc->VirtualStart,
			// 			  desc->NumberOfPages,
			// 			  desc->Attribute);
			// #endif
		}
	}

	VOID InitializeMemory(EFI_SYSTEM_TABLE *SystemTable)
	{
		EFI_MEMORY_DESCRIPTOR *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)bInfo.EFI.MemoryMap.BaseAddress;
		UINTN NumberOfEntries = bInfo.EFI.MemoryMap.NumberOfEntries;
		UINTN DescriptorSize = bInfo.EFI.MemoryMap.DescriptorSize;

		EFI_STATUS Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, NumberOfEntries * DescriptorSize, (void **)&MemoryMap);
		if (EFI_ERROR(Status))
		{
			error("Failed to allocate memory for memory map: %#lx", Status);
			return;
		}

		Status = SystemTable->BootServices->GetMemoryMap(&NumberOfEntries, MemoryMap, &DescriptorSize, NULL, NULL);
		if (EFI_ERROR(Status))
		{
			error("Failed to get memory map: %#lx", Status);
			return;
		}

		InitializeMemoryEntries(MemoryMap, NumberOfEntries, DescriptorSize);
	}

	// VOID InitializeMemoryNoBS()
	// {
	// 	EFI_MEMORY_DESCRIPTOR *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)bInfo.EFI.MemoryMap.BaseAddress;
	// 	UINTN NumberOfEntries = bInfo.EFI.MemoryMap.NumberOfEntries;
	// 	UINTN DescriptorSize = bInfo.EFI.MemoryMap.DescriptorSize;
	// 	InitializeMemoryEntries(MemoryMap, NumberOfEntries, DescriptorSize);
	// }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	// #ifdef DEBUG
	// 	debug("efi info: %x", bInfo.EFI.Info.raw);
	// 	if (bInfo.EFI.Info.ST && validST)
	// 	{
	// 		EFI_GUID EfiAcpi20Table = EFI_ACPI_20_TABLE_GUID;
	// 		EFI_GUID AcpiTable = ACPI_TABLE_GUID;
	// 		EFI_GUID SalSystemTable = SAL_SYSTEM_TABLE_GUID;
	// 		EFI_GUID SmbiosTable = SMBIOS_TABLE_GUID;
	// 		EFI_GUID Smbios3Table = SMBIOS3_TABLE_GUID;
	// 		EFI_GUID MpsTable = MPS_TABLE_GUID;
	// 		EFI_GUID EfiAcpiTable = EFI_ACPI_TABLE_GUID;
	// 		EFI_GUID EfiLzmaCompressed = EFI_LZMA_COMPRESSED_GUID;
	// 		EFI_GUID EfiDxeServices = EFI_DXE_SERVICES_GUID;
	// 		EFI_GUID EfiHobList = EFI_HOB_LIST_GUID;
	// 		EFI_GUID EfiMemoryType = _EFI_MEMORY_TYPE_GUID;
	// 		EFI_GUID EfiDebugImageInfoTable = EFI_DEBUG_IMAGE_INFO_TABLE_GUID;
	// 		EFI_GUID EfiMemStatusCodeRec = EFI_MEM_STATUS_CODE_REC_GUID;
	// 		EFI_GUID EfiGuidEfiAcpi1 = EFI_GUID_EFI_ACPI1_GUID;
	// 		EFI_GUID EfiMemoryAttributesTable = EFI_MEMORY_ATTRIBUTES_TABLE_GUID;
	// 		EFI_GUID EfiHiiDatabaseProtocol = EFI_HII_DATABASE_PROTOCOL_GUID;
	// 		EFI_GUID EfiHiiConfigRoutingProtocol = EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID;
	// 		EFI_GUID TCG2FinalEventsTable = TCG2_FINAL_EVENTS_TABLE_GUID;
	// 		EFI_GUID EfiImageSecurityDatabase = EFI_IMAGE_SECURITY_DATABASE_GUID;
	// 		EFI_GUID EfiSystemResourceTable = EFI_SYSTEM_RESOURCE_TABLE_GUID;
	// 		debug("there are %d configuration tables", SystemTable->NumberOfTableEntries);
	// 		for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++)
	// 		{
	// 			EFI_CONFIGURATION_TABLE *config = &SystemTable->ConfigurationTable[i];
	// 			EFI_GUID *g = &config->VendorGuid;
	// 			void *addr = config->VendorTable;
	// 			const char *guid_str = NULL;
	// 			if (CompareGuid(g, &EfiAcpi20Table))
	// 				guid_str = "EFI ACPI 2.0 Table";
	// 			else if (CompareGuid(g, &AcpiTable))
	// 				guid_str = "ACPI Table";
	// 			else if (CompareGuid(g, &SalSystemTable))
	// 				guid_str = "SAL System Table";
	// 			else if (CompareGuid(g, &SmbiosTable))
	// 				guid_str = "SMBIOS Table";
	// 			else if (CompareGuid(g, &Smbios3Table))
	// 				guid_str = "SMBIOS 3 Table";
	// 			else if (CompareGuid(g, &MpsTable))
	// 				guid_str = "MPS Table";
	// 			else if (CompareGuid(g, &EfiAcpiTable))
	// 				guid_str = "EFI ACPI Table";
	// 			else if (CompareGuid(g, &EfiLzmaCompressed))
	// 				guid_str = "EFI LZMA Compressed";
	// 			else if (CompareGuid(g, &EfiDxeServices))
	// 				guid_str = "EFI DXE Services";
	// 			else if (CompareGuid(g, &EfiHobList))
	// 				guid_str = "EFI HOB List";
	// 			else if (CompareGuid(g, &EfiMemoryType))
	// 				guid_str = "EFI Memory Type";
	// 			else if (CompareGuid(g, &EfiDebugImageInfoTable))
	// 				guid_str = "EFI Debug Image Info Table";
	// 			else if (CompareGuid(g, &EfiMemStatusCodeRec))
	// 				guid_str = "EFI Memory Status Code Record";
	// 			else if (CompareGuid(g, &EfiGuidEfiAcpi1))
	// 				guid_str = "EFI ACPI 1.0 Table";
	// 			else if (CompareGuid(g, &EfiMemoryAttributesTable))
	// 				guid_str = "EFI Memory Attributes Table";
	// 			else if (CompareGuid(g, &EfiHiiDatabaseProtocol))
	// 				guid_str = "EFI HII Database Protocol";
	// 			else if (CompareGuid(g, &EfiHiiConfigRoutingProtocol))
	// 				guid_str = "EFI HII Config Routing Protocol";
	// 			else if (CompareGuid(g, &TCG2FinalEventsTable))
	// 				guid_str = "TCG2 Final Events Table";
	// 			else if (CompareGuid(g, &EfiImageSecurityDatabase))
	// 				guid_str = "EFI Image Security Database";
	// 			else if (CompareGuid(g, &EfiSystemResourceTable))
	// 				guid_str = "EFI System Resource Table";
	// 			else
	// 				guid_str = "(unknown)";
	// 			debug("%016lx  %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x  %s",
	// 				  (UINT64)(uintptr_t)addr,
	// 				  g->Data1, g->Data2, g->Data3,
	// 				  g->Data4[0], g->Data4[1],
	// 				  g->Data4[2], g->Data4[3],
	// 				  g->Data4[4], g->Data4[5],
	// 				  g->Data4[6], g->Data4[7],
	// 				  guid_str);
	// 		}
	// 	}
	// #endif

	SearchSMBIOS(SystemTable);
	SearchRSDP(SystemTable);

	// if (bInfo.EFI.Info.BS == 0 && bInfo.EFI.Info.MemoryMap == 1)
	// 	InitializeMemoryNoBS();

	InitializeMemory(SystemTable);

	/* ... */

	return EFI_SUCCESS;
}
