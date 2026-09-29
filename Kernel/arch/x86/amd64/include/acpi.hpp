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

#ifndef __FENNIX_KERNEL_ACPI_H__
#define __FENNIX_KERNEL_ACPI_H__

#include <types.h>

#include <unordered_map>
#include <boot/binfo.h>
#include <cpu.hpp>
#include <irq.hpp>
#include <vector>

namespace Platform
{
	struct ACPIHeader
	{
		unsigned char Signature[4];
		uint32_t Length;
		uint8_t Revision;
		uint8_t Checksum;
		uint8_t OEMID[6];
		uint8_t OEMTableID[8];
		uint32_t OEMRevision;
		uint32_t CreatorID;
		uint32_t CreatorRevision;
	} __packed;

	class ACPI
	{
	public:
		struct GenericAddressStructure
		{
			uint8_t AddressSpace;
			uint8_t BitWidth;
			uint8_t BitOffset;
			uint8_t AccessSize;
			uint64_t Address;
		} __packed;

		enum DBG2PortType
		{
			TYPE_SERIAL = 0x8000,
			TYPE_1394 = 0x8001,
			TYPE_USB = 0x8002,
			TYPE_NET = 0x8003
		};

		enum DBG2PortSubtype
		{
			SUBTYPE_SERIAL_16550_COMPATIBLE = 0x0000,
			SUBTYPE_SERIAL_16550_SUBSET = 0x0001,
			SUBTYPE_SERIAL_MAX311xE_SPI_UART = 0x0002,
			SUBTYPE_SERIAL_Arm_PL011_UART = 0x0003,
			SUBTYPE_SERIAL_MSM8x60 = 0x0004,
			SUBTYPE_SERIAL_Nvidia_16550 = 0x0005,
			SUBTYPE_SERIAL_TI_OMAP = 0x0006,
			SUBTYPE_SERIAL_APM88xxxx = 0x0008,
			SUBTYPE_SERIAL_MSM8974 = 0x0009,
			SUBTYPE_SERIAL_SAM5250 = 0x000A,
			SUBTYPE_SERIAL_Intel_USIF = 0x000B,
			SUBTYPE_SERIAL_iMX6 = 0x000C,
			SUBTYPE_SERIAL_Arm_SBSA_UART = 0x000D,
			SUBTYPE_SERIAL_Arm_SBSA_Generic_UART = 0x000E,
			SUBTYPE_SERIAL_Arm_DCC = 0x000F,
			SUBTYPE_SERIAL_BCM2835 = 0x0010,
			SUBTYPE_SERIAL_SDM845_At_1_8432MHz = 0x0011,
			SUBTYPE_SERIAL_16550_With_Generic_Address_Structure = 0x0012,
			SUBTYPE_SERIAL_SDM845_At_7_372MHz = 0x0013,
			SUBTYPE_SERIAL_Intel_LPSS = 0x0014,
			SUBTYPE_SERIAL_RISC_V_SBI_Console = 0x0015,

			SUBTYPE_1394_IEEE1394_HCI = 0x0000,

			SUBTYPE_USB_XHCI = 0x0000,
			SUBTYPE_USB_EHCI = 0x0001,

			SUBTYPE_NET_NNNN = 0x0000,
		};

		struct DBG2Device
		{
			uint8_t Revision;
			uint16_t Length;
			uint8_t NumberofGenericAddressRegisters;
			uint16_t NamespaceStringLength;
			uint16_t NamespaceStringOffset;
			uint16_t OemDataLength;
			uint16_t OemDataOffset;
			uint16_t PortType;
			uint16_t PortSubtype;
			uint16_t Reserved;
			uint16_t BaseAddressRegisterOffset;
			uint16_t AddressSizeOffset;
			/* BaseAddressRegister[NumberofGenericAddressRegisters * 12] at offset BaseAddressRegisterOffset */
			/* AddressSize[NumberofGenericAddressRegisters * 4] at offset AddressSizeOffset */
			/* NamespaceString[NamespaceStringLength] at offset NamespaceStringOffset */
			/* OemData[OemDataLength] at offset OemDataOffset */
		} __packed;

		struct MCFGHeader
		{
			ACPIHeader Header;
			uint64_t Reserved;
		} __packed;

		struct HPETHeader
		{
			ACPIHeader Header;
			uint8_t HardwareRevID;
			uint8_t ComparatorCount : 5;
			uint8_t CounterSize : 1;
			uint8_t Reserved : 1;
			uint8_t LegacyReplacement : 1;
			uint16_t PCIVendorID;
			struct GenericAddressStructure Address;
			uint8_t HPETNumber;
			uint16_t MinimumTick;
			uint8_t PageProtection;
		} __packed;

		struct FADTHeader
		{
			ACPIHeader Header;
			uint32_t FirmwareCtrl;
			uint32_t Dsdt;
			uint8_t Reserved;
			uint8_t PreferredPowerManagementProfile;
			uint16_t SCI_Interrupt;
			uint32_t SMI_CommandPort;
			uint8_t AcpiEnable;
			uint8_t AcpiDisable;
			uint8_t S4BIOS_REQ;
			uint8_t PSTATE_Control;
			uint32_t PM1aEventBlock;
			uint32_t PM1bEventBlock;
			uint32_t PM1aControlBlock;
			uint32_t PM1bControlBlock;
			uint32_t PM2ControlBlock;
			uint32_t PMTimerBlock;
			uint32_t GPE0Block;
			uint32_t GPE1Block;
			uint8_t PM1EventLength;
			uint8_t PM1ControlLength;
			uint8_t PM2ControlLength;
			uint8_t PMTimerLength;
			uint8_t GPE0Length;
			uint8_t GPE1Length;
			uint8_t GPE1Base;
			uint8_t CStateControl;
			uint16_t WorstC2Latency;
			uint16_t WorstC3Latency;
			uint16_t FlushSize;
			uint16_t FlushStride;
			uint8_t DutyOffset;
			uint8_t DutyWidth;
			uint8_t DayAlarm;
			uint8_t MonthAlarm;
			uint8_t Century;
			uint16_t BootArchitectureFlags;
			uint8_t Reserved2;
			uint32_t Flags;
			struct GenericAddressStructure ResetReg;
			uint8_t ResetValue;
			uint8_t Reserved3[3];
			uint64_t X_FirmwareControl;
			uint64_t X_Dsdt;
			struct GenericAddressStructure X_PM1aEventBlock;
			struct GenericAddressStructure X_PM1bEventBlock;
			struct GenericAddressStructure X_PM1aControlBlock;
			struct GenericAddressStructure X_PM1bControlBlock;
			struct GenericAddressStructure X_PM2ControlBlock;
			struct GenericAddressStructure X_PMTimerBlock;
			struct GenericAddressStructure X_GPE0Block;
			struct GenericAddressStructure X_GPE1Block;
		} __packed;

		struct BGRTHeader
		{
			ACPIHeader Header;

			/**
			 * Version. This value must be 1.
			 */
			uint16_t Version;

			/**
			 * Status of the image
			 */
			union
			{
				struct
				{
					/**
					 * Indicates that the image graphic is displayed.
					 */
					uint8_t Displayed : 1;

					/**
					 * Orientation
					 *
					 * 0b00 - 0˚
					 * 0b01 - 90˚
					 * 0b10 - 180˚
					 * 0b11 - 270˚
					 */
					uint8_t OrientationOffset : 2;

					/**
					 * This field is reserved and must be zero.
					 */
					uint8_t Reserved : 5;
				};
				uint8_t raw;
			} Status;

			/**
			 * Image type
			 *
			 * 0 - Bitmap
			 * 1-255 - Reserved
			 */
			uint8_t ImageType;

			/**
			 * Physical address of the image pointing to firmware's in-memory copy of the image bitmap.
			 */
			uint64_t ImageAddress;

			/**
			 * X-offset of the boot image.
			 */
			uint32_t ImageOffsetX;

			/**
			 * Y-offset of the boot image.
			 */
			uint32_t ImageOffsetY;
		} __packed;

		struct SRATHeader
		{
			ACPIHeader Header;
			uint32_t TableRevision; // Must be value 1
			uint64_t Reserved;		// Reserved, must be zero
		} __packed;

		struct TPM2Header
		{
			ACPIHeader Header;
			uint32_t Flags;
			uint64_t ControlAddress;
			uint32_t StartMethod;
		} __packed;

		struct TCPAHeader
		{
			ACPIHeader Header;
			uint16_t Reserved;
			uint32_t MaxLogLength;
			uint64_t LogAddress;
		} __packed;

		struct WAETHeader
		{
			ACPIHeader Header;
			uint32_t Flags;
		} __packed;

		struct HESTHeader
		{
			ACPIHeader Header;
			uint32_t ErrorSourceCount;
		} __packed;

		struct MADTHeader
		{
			ACPIHeader Header;
			uint32_t LocalControllerAddress;
			uint32_t Flags;
			char Entries[];
		} __packed;

		struct SSDTHeader
		{
			ACPIHeader Header;
			char DefinitionBlock[];
		} __packed;

		struct DBGPHeader
		{
			ACPIHeader Header;
			/**
			 * 0 - 16550 compatible
			 * 1 - Subset of 16550
			 */
			uint8_t InterfaceType;
			uint8_t Reserved[3];
			GenericAddressStructure BaseAddress;
		} __packed;

		struct DBG2Header
		{
			ACPIHeader Header;
			uint32_t OffsetDbgDeviceInfo;
			uint32_t NumberDbgDeviceInfo;
			/* DBG2Device[NumberDbgDeviceInfo] at offset OffsetDbgDeviceInfo */
		} __packed;

		ACPIHeader *XSDT = nullptr;
		MCFGHeader *MCFG = nullptr;
		HPETHeader *HPET = nullptr;
		FADTHeader *FADT = nullptr;
		BGRTHeader *BGRT = nullptr;
		SRATHeader *SRAT = nullptr;
		TPM2Header *TPM2 = nullptr;
		TCPAHeader *TCPA = nullptr;
		WAETHeader *WAET = nullptr;
		MADTHeader *MADT = nullptr;
		HESTHeader *HEST = nullptr;
		SSDTHeader *SSDT = nullptr;
		DBGPHeader *DBGP = nullptr;
		DBG2Header *DBG2 = nullptr;
		bool XSDTSupported = false;

		std::unordered_map<const char *, ACPIHeader *> Tables;

		void *FindTable(ACPIHeader *Header, char *Signature);
		void SearchTables(ACPIHeader *Header);

		bool IsPresent() { return !Tables.empty(); }

		ACPI();
		~ACPI() = default;
	};

	class MADT
	{
	public:
		struct APICHeader
		{
			uint8_t Type;
			uint8_t Length;
		} __packed;

		struct MADTIOApic
		{
			struct APICHeader Header;
			uint8_t APICID;
			uint8_t reserved;
			uint32_t Address;
			uint32_t GSIBase;
		} __packed;

		struct MADTIso
		{
			struct APICHeader Header;
			uint8_t BuSSource;
			uint8_t IRQSource;
			uint32_t GSI;
			uint16_t Flags;
		} __packed;

		struct MADTNmi
		{
			struct APICHeader Header;
			uint8_t processor;
			uint16_t flags;
			uint8_t lint;
		} __packed;

		struct LocalAPIC
		{
			struct APICHeader Header;
			uint8_t ACPIProcessorId;
			uint8_t APICId;
			uint32_t Flags;
		} __packed;

		struct LAPIC
		{
			uint8_t id;
			uintptr_t PhysicalAddress;
			void *VirtualAddress;
		};

		std::vector<MADTIOApic *> ioapic;
		std::vector<MADTIso *> iso;
		std::vector<MADTNmi *> nmi;
		std::vector<LocalAPIC *> lapic;
		struct LAPIC *LAPICAddress;
		uint16_t CPUCores;

		MADT(ACPI::MADTHeader *madt);
		~MADT() = default;
	};

	class DSDT : public Interrupt::Handler
	{
	private:
		uint32_t SMI_CMD = 0;
		uint8_t ACPI_ENABLE = 0;
		uint8_t ACPI_DISABLE = 0;
		uint32_t PM1a_CNT = 0;
		uint32_t PM1b_CNT = 0;
		uint16_t SLP_TYPa = 0;
		uint16_t SLP_TYPb = 0;
		uint16_t SLP_EN = 0;
		uint16_t SCI_EN = 0;
		uint8_t PM1_CNT_LEN = 0;

		ACPI *acpi;
		int OnInterruptReceived(CPU::TrapFrame *Frame);

	public:
		bool ACPIShutdownSupported = false;

		void Reboot();
		void Shutdown();

		DSDT(ACPI *acpi);
		~DSDT() = default;
	};
}

#endif // !__FENNIX_KERNEL_ACPI_H__
