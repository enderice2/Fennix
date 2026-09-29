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

#ifndef __FENNIX_KERNEL_SMBIOS_H__
#define __FENNIX_KERNEL_SMBIOS_H__

#include <types.h>

namespace SMBIOS
{
	enum SMBIOSType
	{
		SMBIOSTypeBIOSInformation = 0,
		SMBIOSTypeSystemInformation = 1,
		SMBIOSTypeBaseBoardInformation = 2,
		SMBIOSTypeSystemEnclosure = 3,
		SMBIOSTypeProcessorInformation = 4,
		SMBIOSTypeMemoryControllerInformation = 5,
		SMBIOSTypeMemoryModuleInformation = 6,
		SMBIOSTypeCacheInformation = 7,
		SMBIOSTypePortConnectorInformation = 8,
		SMBIOSTypeSystemSlots = 9,
		SMBIOSTypeOnBoardDevicesInformation = 10,
		SMBIOSTypeOEMStrings = 11,
		SMBIOSTypeSystemConfigurationOptions = 12,
		SMBIOSTypeBIOSLanguageInformation = 13,
		SMBIOSTypeGroupAssociations = 14,
		SMBIOSTypeSystemEventLog = 15,
		SMBIOSTypePhysicalMemoryArray = 16,
		SMBIOSTypeMemoryDevice = 17,
		SMBIOSType32BitMemoryErrorInformation = 18,
		SMBIOSTypeMemoryArrayMappedAddress = 19,
		SMBIOSTypeMemoryDeviceMappedAddress = 20,
		SMBIOSTypeBuiltInPointingDevice = 21,
		SMBIOSTypePortableBattery = 22,
		SMBIOSTypeSystemReset = 23,
		SMBIOSTypeHardwareSecurity = 24,
		SMBIOSTypeSystemPowerControls = 25,
		SMBIOSTypeVoltageProbe = 26,
		SMBIOSTypeCoolingDevice = 27,
		SMBIOSTypeTemperatureProbe = 28,
		SMBIOSTypeElectricalCurrentProbe = 29,
		SMBIOSTypeOutofBandRemoteAccess = 30,
		SMBIOSTypeBootIntegrityServices = 31,
		SMBIOSTypeSystemBoot = 32,
		SMBIOSType64BitMemoryErrorInformation = 33,
		SMBIOSTypeManagementDevice = 34,
		SMBIOSTypeManagementDeviceComponent = 35,
		SMBIOSTypeManagementDeviceThresholdData = 36,
		SMBIOSTypeMemoryChannel = 37,
		SMBIOSTypeIPMIDevice = 38,
		SMBIOSTypePowerSupply = 39,
		SMBIOSTypeAdditionalInformation = 40,
		SMBIOSTypeOnboardDevicesExtendedInformation = 41,
		SMBIOSTypeManagementControllerHostInterface = 42,
		SMBIOSTypeTPMDevice = 43,
		SMBIOSTypeProcessorAdditionalInformation = 44,
		SMBIOSTypeInactive = 126,
		SMBIOSTypeEndOfTable = 127
	};

	struct SMBIOSHeader
	{
		unsigned char Type;
		unsigned char Length;
		unsigned short Handle;
	};

	struct SMBIOSEntryPoint
	{
		char EntryPointString[4];
		unsigned char Checksum;
		unsigned char Length;
		unsigned char MajorVersion;
		unsigned char MinorVersion;
		unsigned short MaxStructureSize;
		unsigned char EntryPointRevision;
		char FormattedArea[5];
		char EntryPointString2[5];
		unsigned char Checksum2;
		unsigned short TableLength;
		unsigned int TableAddress;
		unsigned short NumberOfStructures;
		unsigned char BCDRevision;
	};

	static inline char *SMBIOSNextString(char *Str)
	{
		while (*Str != '\0')
			Str++;
		return Str + 1;
	}

	struct SMBIOSBIOSInformation
	{
		SMBIOSHeader Header;
		unsigned char Vendor;
		unsigned char Version;
		unsigned short StartingAddressSegment;
		unsigned char ReleaseDate;
		unsigned char ROMSize;
		unsigned long Characteristics;
		unsigned char CharacteristicsExtensionBytes[2];
		unsigned char SystemBIOSMajorRelease;
		unsigned char SystemBIOSMinorRelease;
		unsigned char EmbeddedControllerFirmwareMajorRelease;
		unsigned char EmbeddedControllerFirmwareMinorRelease;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSSystemInformation
	{
		SMBIOSHeader Header;
		unsigned char Manufacturer;
		unsigned char ProductName;
		unsigned char Version;
		unsigned char SerialNumber;
		unsigned char UUID[16];
		unsigned char WakeUpType;
		unsigned char SKU;
		unsigned char Family;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSBaseBoardInformation
	{
		SMBIOSHeader Header;
		unsigned char Manufacturer;
		unsigned char Product;
		unsigned char Version;
		unsigned char SerialNumber;
		unsigned char AssetTag;
		unsigned char FeatureFlags;
		unsigned char LocationInChassis;
		unsigned short ChassisHandle;
		unsigned char BoardType;
		unsigned char NumberOfContainedObjectHandles;
		unsigned short ContainedObjectHandles[0];

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSProcessorInformation
	{
		SMBIOSHeader Header;
		unsigned char SocketDesignation;
		unsigned char ProcessorType;
		unsigned char ProcessorFamily;
		unsigned char ProcessorManufacturer;
		unsigned long ProcessorID;
		unsigned char ProcessorVersion;
		unsigned char Voltage;
		unsigned short ExternalClock;
		unsigned short MaxSpeed;
		unsigned short CurrentSpeed;
		unsigned char Status;
		unsigned char ProcessorUpgrade;
		unsigned short L1CacheHandle;
		unsigned short L2CacheHandle;
		unsigned short L3CacheHandle;
		unsigned char SerialNumber;
		unsigned char AssetTag;
		unsigned char PartNumber;
		unsigned char CoreCount;
		unsigned char CoreEnabled;
		unsigned char ThreadCount;
		unsigned short ProcessorCharacteristics;
		unsigned short ProcessorFamily2;
		unsigned short CoreCount2;
		unsigned short CoreEnabled2;
		unsigned short ThreadCount2;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSMemoryDevice
	{
		SMBIOSHeader Header;
		unsigned char PhysicalMemoryArrayHandle;
		unsigned char MemoryErrorInformationHandle;
		unsigned short TotalWidth;
		unsigned short DataWidth;
		unsigned short Size;
		unsigned char FormFactor;
		unsigned char DeviceSet;
		unsigned char DeviceLocator;
		unsigned char BankLocator;
		unsigned char MemoryType;
		unsigned short TypeDetail;
		unsigned short Speed;
		unsigned char Manufacturer;
		unsigned char SerialNumber;
		unsigned char AssetTag;
		unsigned char PartNumber;
		unsigned char Attributes;
		unsigned short ExtendedSize;
		unsigned short ConfiguredMemoryClockSpeed;
		unsigned short MinimumVoltage;
		unsigned short MaximumVoltage;
		unsigned short ConfiguredVoltage;
		unsigned char MemoryTechnology;
		unsigned char OperatingModeCapability;
		unsigned char FirmwareVersion;
		unsigned char ModuleManufacturerID;
		unsigned char ModuleProductID;
		unsigned char MemorySubsystemControllerManufacturerID;
		unsigned char MemorySubsystemControllerProductID;
		unsigned short NonVolatileSize;
		unsigned short VolatileSize;
		unsigned short CacheSize;
		unsigned short LogicalSize;
		unsigned char ExtendedSpeed;
		unsigned char ExtendedConfiguredMemorySpeed;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSMemoryArrayMappedAddress
	{
		SMBIOSHeader Header;
		unsigned int StartingAddress;
		unsigned int EndingAddress;
		unsigned short MemoryArrayHandle;
		unsigned char PartitionWidth;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSMemoryDeviceMappedAddress
	{
		SMBIOSHeader Header;
		unsigned int StartingAddress;
		unsigned int EndingAddress;
		unsigned short MemoryDeviceHandle;
		unsigned short MemoryArrayMappedAddressHandle;
		unsigned char PartitionRowPosition;
		unsigned char InterleavePosition;
		unsigned char InterleavedDataDepth;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	struct SMBIOSMemoryArray
	{
		SMBIOSHeader Header;
		unsigned char Location;
		unsigned char Use;
		unsigned char MemoryErrorCorrection;
		unsigned int MaximumCapacity;
		unsigned short MemoryErrorInformationHandle;
		unsigned short NumberOfMemoryDevices;

		const char *GetString(unsigned char Index)
		{
			char *Str = (char *)((unsigned long)this + this->Header.Length);
			Index--;
			if (Index == 0 || Index > 10)
				return Str;
			for (unsigned char i = 0; i < Index; i++)
				Str = SMBIOSNextString(Str);
			return Str;
		}
	};

	bool CheckSMBIOS();
	SMBIOSEntryPoint *GetSMBIOSEntryPoint();
	void *GetSMBIOSHeader(SMBIOSType Type);
	SMBIOSBIOSInformation *GetBIOSInformation();
	SMBIOSSystemInformation *GetSystemInformation();
	SMBIOSBaseBoardInformation *GetBaseBoardInformation();
	SMBIOSProcessorInformation *GetProcessorInformation();
	SMBIOSMemoryArray *GetMemoryArray();
	SMBIOSMemoryDevice *GetMemoryDevice();
	SMBIOSMemoryArrayMappedAddress *GetMemoryArrayMappedAddress();
	SMBIOSMemoryDeviceMappedAddress *GetMemoryDeviceMappedAddress();
}

#endif // !__FENNIX_KERNEL_SMBIOS_H__
