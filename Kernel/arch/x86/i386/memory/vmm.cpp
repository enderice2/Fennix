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

#include <memory.hpp>

#include <convert.h>
#include <debug.h>

namespace Memory
{
	bool Virtual::Check(void *VirtualAddress, PTFlag Flag, MapType Type)
	{
		// 0x1000 aligned
		uintptr_t Address = (uintptr_t)VirtualAddress;
		Address &= 0xFFFFF000;

		PageMapIndexer Index = PageMapIndexer(Address);
		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		PageTableEntryPtr *PTE = nullptr;

		if ((PDE->raw & Flag) > 0)
		{
			if (Type == MapType::FourMiB && PDE->PageSize)
				return true;

			PTE = (PageTableEntryPtr *)((uintptr_t)PDE->GetAddress() << 12);
			if (PTE)
			{
				if ((PTE->Entries[Index.PTEIndex].Present))
					return true;
			}
		}
		return false;
	}

	void *Virtual::GetPhysical(void *VirtualAddress)
	{
		// 0x1000 aligned
		uintptr_t Address = (uintptr_t)VirtualAddress;
		Address &= 0xFFFFF000;

		PageMapIndexer Index = PageMapIndexer(Address);
		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		PageTableEntryPtr *PTE = nullptr;

		if (PDE->Present)
		{
			if (PDE->PageSize)
				return (void *)((uintptr_t)PDE->GetAddress() << 12);

			PTE = (PageTableEntryPtr *)((uintptr_t)PDE->GetAddress() << 12);
			if (PTE)
			{
				if (PTE->Entries[Index.PTEIndex].Present)
					return (void *)((uintptr_t)PTE->Entries[Index.PTEIndex].GetAddress() << 12);
			}
		}
		return nullptr;
	}

	Virtual::MapType Virtual::GetMapType(void *VirtualAddress)
	{
		// 0x1000 aligned
		uintptr_t Address = (uintptr_t)VirtualAddress;
		Address &= 0xFFFFF000;

		PageMapIndexer Index = PageMapIndexer(Address);

		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		PageTableEntryPtr *PTE = nullptr;

		if (PDE->Present)
		{
			if (PDE->PageSize)
				return MapType::FourMiB;

			PTE = (PageTableEntryPtr *)((uintptr_t)PDE->GetAddress() << 12);
			if (PTE)
			{
				if (PTE->Entries[Index.PTEIndex].Present)
					return MapType::FourKiB;
			}
		}
		return MapType::NoMapType;
	}

	PageDirectoryEntry *Virtual::GetPDE(void *VirtualAddress, MapType Type)
	{
		uintptr_t Address = (uintptr_t)VirtualAddress;
		Address &= 0xFFFFF000;

		PageMapIndexer Index = PageMapIndexer(Address);
		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		if (PDE->Present)
			return PDE;
		return nullptr;
	}

	PageTableEntry *Virtual::GetPTE(void *VirtualAddress, MapType Type)
	{
		uintptr_t Address = (uintptr_t)VirtualAddress;
		Address &= 0xFFFFF000;

		PageMapIndexer Index = PageMapIndexer(Address);
		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		if (!PDE->Present)
			return nullptr;

		PageTableEntryPtr *PTEPtr = (PageTableEntryPtr *)(PDE->GetAddress() << 12);
		PageTableEntry *PTE = &PTEPtr->Entries[Index.PTEIndex];
		if (PTE->Present)
			return PTE;
		return nullptr;
	}

	void Virtual::Map(void *VirtualAddress, void *PhysicalAddress, uint64_t Flags, MapType Type)
	{
		SmartLock(this->MemoryLock);
		if (unlikely(!this->pTable))
		{
			error("No page table");
			return;
		}

		Flags |= PTFlag::P;

		PageMapIndexer Index = PageMapIndexer((uintptr_t)VirtualAddress);
		// Clear any flags that are not 1 << 0 (Present) - 1 << 5 (Accessed) because rest are for page table entries only
		uint64_t DirectoryFlags = Flags & 0x3F;

		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		if (Type == MapType::FourMiB)
		{
			PDE->raw |= (uintptr_t)Flags;
			PDE->PageSize = true;
			PDE->SetAddress((uintptr_t)PhysicalAddress >> 12);
			debug("Mapped 4MB page at %p to %p", VirtualAddress, PhysicalAddress);
			return;
		}

		PageTableEntryPtr *PTEPtr = nullptr;
		if (!PDE->Present)
		{
			PTEPtr = (PageTableEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageTableEntryPtr) + 1));
			memset(PTEPtr, 0, sizeof(PageTableEntryPtr));
			PDE->Present = true;
			PDE->SetAddress((uintptr_t)PTEPtr >> 12);
		}
		else
			PTEPtr = (PageTableEntryPtr *)(PDE->GetAddress() << 12);
		PDE->raw |= (uintptr_t)DirectoryFlags;

		PageTableEntry *PTE = &PTEPtr->Entries[Index.PTEIndex];
		PTE->Present = true;
		PTE->raw |= (uintptr_t)Flags;
		PTE->SetAddress((uintptr_t)PhysicalAddress >> 12);
		CPU::x32::invlpg(VirtualAddress);

#ifdef DEBUG
/* https://stackoverflow.com/a/3208376/9352057 */
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
	(byte & 0x80 ? '1' : '0'),     \
		(byte & 0x40 ? '1' : '0'), \
		(byte & 0x20 ? '1' : '0'), \
		(byte & 0x10 ? '1' : '0'), \
		(byte & 0x08 ? '1' : '0'), \
		(byte & 0x04 ? '1' : '0'), \
		(byte & 0x02 ? '1' : '0'), \
		(byte & 0x01 ? '1' : '0')

		if (!this->Check(VirtualAddress, (PTFlag)Flags, Type)) // quick workaround just to see where it fails
			warn("Failed to map v:%#lx p:%#lx with flags: " BYTE_TO_BINARY_PATTERN, VirtualAddress, PhysicalAddress, BYTE_TO_BINARY(Flags));
#endif
	}

	void Virtual::OptimizedMap(fnx::void_t VirtualAddress, fnx::void_t PhysicalAddress, size_t Length, uint64_t Flags)
	{
		static bool PSESupport = false;

		static int once = 0;
		if (!once++)
		{
			if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
			{
				CPU::x86::AMD::CPUID0x80000001 cpuid;
				PSESupport = cpuid.EDX.PSE;
			}
			else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
			{
				CPU::x86::Intel::CPUID0x00000001 cpuid;
				PSESupport = cpuid.EDX.PSE;
			}

			if (PSESupport)
			{
				CPU::x32::CR4 cr4 = CPU::x32::readcr4();
				cr4.PSE = 1;
				CPU::x32::writecr4(cr4);
			}
		}

		if (PSESupport)
		{
			while (Length >= PAGE_SIZE_4M &&
				   is_aligned(VirtualAddress, PAGE_SIZE_4M) &&
				   is_aligned(PhysicalAddress, PAGE_SIZE_4M))
			{
				this->SingleMap(VirtualAddress, PhysicalAddress, Flags, Virtual::MapType::FourMiB);
				VirtualAddress += PAGE_SIZE_4M;
				PhysicalAddress += PAGE_SIZE_4M;
				Length -= PAGE_SIZE_4M;
			}
		}

		while (Length >= PAGE_SIZE_4K)
		{
			this->SingleMap(VirtualAddress, PhysicalAddress, Flags, Virtual::MapType::FourKiB);
			VirtualAddress += PAGE_SIZE_4K;
			PhysicalAddress += PAGE_SIZE_4K;
			Length -= PAGE_SIZE_4K;
		}
	}

	void Virtual::Unmap(fnx::void_t VirtualAddress, MapType Type)
	{
		SmartLock(this->MemoryLock);
		if (!this->pTable)
		{
			error("No page table");
			return;
		}

		PageMapIndexer Index = PageMapIndexer(VirtualAddress);
		PageDirectoryEntry *PDE = &this->pTable->Entries[Index.PDEIndex];
		if (!PDE->Present)
		{
			warn("Page %#lx not present", PDE->GetAddress());
			return;
		}

		if (Type == MapType::FourMiB && PDE->PageSize)
		{
			PDE->Present = false;
			return;
		}

		PageTableEntryPtr *PTEPtr = (PageTableEntryPtr *)((uintptr_t)PDE->Address << 12);
		PageTableEntry PTE = PTEPtr->Entries[Index.PTEIndex];
		if (!PTE.Present)
		{
			warn("Page %#lx not present", PTE.GetAddress());
			return;
		}

		PTE.Present = false;
		PTEPtr->Entries[Index.PTEIndex] = PTE;
		CPU::x32::invlpg(VirtualAddress);
	}

	void Virtual::Remap(fnx::void_t VirtualAddress, fnx::void_t PhysicalAddress, uint64_t Flags, MapType Type)
	{
		SmartLock(this->MemoryLock);
		if (unlikely(!this->pTable))
		{
			error("No page table");
			return;
		}

#warning "Remap is not properly implemented"
		this->Unmap(VirtualAddress, Type);
		this->Map(VirtualAddress, PhysicalAddress, Flags, Type);
	}
}
