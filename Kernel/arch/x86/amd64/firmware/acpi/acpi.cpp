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

#include <debug.h>
#include <io.h>

#include "../kernel.h"

namespace Platform
{
	__no_sanitize("alignment") void *ACPI::FindTable(ACPIHeader *Header, char *Signature)
	{
		for (uint64_t t = 0; t < ((Header->Length - sizeof(ACPIHeader)) / (XSDTSupported ? 8 : 4)); t++)
		{
			// TODO: Should I be concerned about unaligned memory access?
			ACPIHeader *SDTHdr = nullptr;
			uintptr_t entry;
			if (XSDTSupported)
				entry = *(uint64_t *)((uintptr_t)Header + sizeof(ACPIHeader) + (t * 8));
			else
				entry = *(uint32_t *)((uintptr_t)Header + sizeof(ACPIHeader) + (t * 4));
			SDTHdr = (ACPIHeader *)entry;

			size_t signLength = strlen(Signature);
			for (size_t i = 0; i < signLength; i++)
			{
				if (SDTHdr->Signature[i] != Signature[i])
					break;
				if (i == 3)
				{
					klog("ACPI: %.4s [%.6s:%.8s] found at address %#lx",
						 Signature,
						 SDTHdr->OEMID,
						 SDTHdr->OEMTableID,
						 (uintptr_t)SDTHdr);
					Tables[Signature] = SDTHdr;
					return SDTHdr;
				}
			}
		}
		// warn("%s not found!", Signature);
		return nullptr;
	}

	void ACPI::SearchTables(ACPIHeader *Header)
	{
		HPET = (HPETHeader *)FindTable(Header, (char *)"HPET");
		FADT = (FADTHeader *)FindTable(Header, (char *)"FACP");
		MCFG = (MCFGHeader *)FindTable(Header, (char *)"MCFG");
		BGRT = (BGRTHeader *)FindTable(Header, (char *)"BGRT");
		SRAT = (SRATHeader *)FindTable(Header, (char *)"SRAT");
		TPM2 = (TPM2Header *)FindTable(Header, (char *)"TPM2");
		TCPA = (TCPAHeader *)FindTable(Header, (char *)"TCPA");
		WAET = (WAETHeader *)FindTable(Header, (char *)"WAET");
		MADT = (MADTHeader *)FindTable(Header, (char *)"APIC");
		HEST = (HESTHeader *)FindTable(Header, (char *)"HEST");
		SSDT = (SSDTHeader *)FindTable(Header, (char *)"SSDT");
		DBGP = (DBGPHeader *)FindTable(Header, (char *)"DBGP");
		DBG2 = (DBG2Header *)FindTable(Header, (char *)"DBG2");
		FindTable(Header, (char *)"BERT");
		FindTable(Header, (char *)"CPEP");
		FindTable(Header, (char *)"DSDT");
		FindTable(Header, (char *)"ECDT");
		FindTable(Header, (char *)"EINJ");
		FindTable(Header, (char *)"ERST");
		FindTable(Header, (char *)"FACS");
		FindTable(Header, (char *)"MSCT");
		FindTable(Header, (char *)"MPST");
		FindTable(Header, (char *)"OEMx");
		FindTable(Header, (char *)"PMTT");
		FindTable(Header, (char *)"PSDT");
		FindTable(Header, (char *)"RASF");
		FindTable(Header, (char *)"RSDT");
		FindTable(Header, (char *)"SBST");
		FindTable(Header, (char *)"SLIT");
		FindTable(Header, (char *)"XSDT");
		FindTable(Header, (char *)"DRTM");
		FindTable(Header, (char *)"FPDT");
		FindTable(Header, (char *)"GTDT");
		FindTable(Header, (char *)"PCCT");
		FindTable(Header, (char *)"S3PT");
		FindTable(Header, (char *)"MATR");
		FindTable(Header, (char *)"MSDM");
		FindTable(Header, (char *)"WPBT");
		FindTable(Header, (char *)"OSDT");
		FindTable(Header, (char *)"RSDP");
		FindTable(Header, (char *)"NFIT");
		FindTable(Header, (char *)"ASF!");
		FindTable(Header, (char *)"BOOT");
		FindTable(Header, (char *)"CSRT");
		FindTable(Header, (char *)"BDAT");
		FindTable(Header, (char *)"CDAT");
		FindTable(Header, (char *)"DMAR");
		FindTable(Header, (char *)"IBFT");
		FindTable(Header, (char *)"IORT");
		FindTable(Header, (char *)"IVRS");
		FindTable(Header, (char *)"LPIT");
		FindTable(Header, (char *)"MCHI");
		FindTable(Header, (char *)"MTMR");
		FindTable(Header, (char *)"SLIC");
		FindTable(Header, (char *)"SPCR");
		FindTable(Header, (char *)"SPMI");
		FindTable(Header, (char *)"UEFI");
		FindTable(Header, (char *)"VRTC");
		FindTable(Header, (char *)"WDAT");
		FindTable(Header, (char *)"WDDT");
		FindTable(Header, (char *)"WDRT");
		FindTable(Header, (char *)"ATKG");
		FindTable(Header, (char *)"GSCI");
		FindTable(Header, (char *)"IEIT");
		FindTable(Header, (char *)"HMAT");
		FindTable(Header, (char *)"CEDT");
		FindTable(Header, (char *)"AEST");
		FindTable(Header, (char *)"AGDI");
		FindTable(Header, (char *)"APMT");
		FindTable(Header, (char *)"ETDT");
		FindTable(Header, (char *)"MPAM");
		FindTable(Header, (char *)"PDTT");
		FindTable(Header, (char *)"PPTT");
		FindTable(Header, (char *)"RAS2");
		FindTable(Header, (char *)"SDEI");
		FindTable(Header, (char *)"STAO");
		FindTable(Header, (char *)"XENV");
		FindTable(Header, (char *)"PHAT");
		FindTable(Header, (char *)"SVKL");
		FindTable(Header, (char *)"UUID");
		FindTable(Header, (char *)"CCEL");
		FindTable(Header, (char *)"WSMT");
		FindTable(Header, (char *)"PRMT");
		FindTable(Header, (char *)"NBFT");
		FindTable(Header, (char *)"RSD PTR");
		FindTable(Header, (char *)"TDX");
		FindTable(Header, (char *)"CXL");
		FindTable(Header, (char *)"DSD");
		FindTable(Header, (char *)"BSA");
	}

	ACPI::ACPI()
	{
		trace("Initializing ACPI");
		if (!bInfo.RSDP)
		{
			error("RSDP not found!");
			return;
		}

		if (bInfo.RSDP->Revision >= 2 && bInfo.RSDP->XSDTAddress)
		{
			debug("XSDT supported");
			XSDTSupported = true;
			XSDT = (ACPIHeader *)(bInfo.RSDP->XSDTAddress);
		}
		else
		{
			debug("RSDT supported");
			XSDT = (ACPIHeader *)(uintptr_t)bInfo.RSDP->RSDTAddress;
		}

		Memory::Virtual vmm;
		if (!vmm.Check(XSDT))
		{
			warn("%s is not mapped!", XSDTSupported ? "XSDT" : "RSDT");
			debug("XSDT: %p", XSDT);
			vmm.SingleMap(XSDT, XSDT, Memory::RW | Memory::PCD | Memory::PWT);
		}

		if (FADT)
		{
			outb(static_cast<uint16_t>(FADT->SMI_CommandPort), FADT->AcpiEnable);
			/* TODO: Sleep for ~5 seconds before polling PM1a CB? */
			while (!(inw(static_cast<uint16_t>(FADT->PM1aControlBlock)) & 1))
				CPU::Pause();
		}

		this->SearchTables(XSDT);
	}
}
