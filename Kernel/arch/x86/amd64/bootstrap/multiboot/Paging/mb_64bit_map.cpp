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

#include <types.h>

union __attribute__((packed)) PageTableEntry
{
	struct
	{
		bool Present : 1;            // 0
		bool ReadWrite : 1;          // 1
		bool UserSupervisor : 1;     // 2
		bool WriteThrough : 1;       // 3
		bool CacheDisable : 1;       // 4
		bool Accessed : 1;           // 5
		bool Dirty : 1;              // 6
		bool PageAttributeTable : 1; // 7
		bool Global : 1;             // 8
		uint8_t Available0 : 3;      // 9-11
		uint64_t Address : 40;       // 12-51
		uint32_t Available1 : 7;     // 52-58
		uint8_t ProtectionKey : 4;   // 59-62
		bool ExecuteDisable : 1;     // 63
	};
	uint64_t raw;

	__always_inline inline nsa nif void SetAddress(uintptr_t _Address)
	{
		_Address &= 0x000000FFFFFFFFFF;
		this->raw &= 0xFFF0000000000FFF;
		this->raw |= (_Address << 12);
	}

	__always_inline inline nsa nif uintptr_t GetAddress() { return (this->raw & 0x000FFFFFFFFFF000) >> 12; }
};

struct __attribute__((packed)) PageTableEntryPtr
{
	PageTableEntry Entries[512];
};

union __attribute__((packed)) PageDirectoryEntry
{
	struct
	{
		bool Present : 1;         // 0
		bool ReadWrite : 1;       // 1
		bool UserSupervisor : 1;  // 2
		bool WriteThrough : 1;    // 3
		bool CacheDisable : 1;    // 4
		bool Accessed : 1;        // 5
		bool Available0 : 1;      // 6
		bool PageSize : 1;        // 7
		uint8_t Available1 : 4;   // 8-11
		uint64_t Address : 40;    // 12-51
		uint32_t Available2 : 11; // 52-62
		bool ExecuteDisable : 1;  // 63
	};
	uint64_t raw;

	__always_inline inline nsa nif void SetAddress(uintptr_t _Address)
	{
		_Address &= 0x000000FFFFFFFFFF;
		this->raw &= 0xFFF0000000000FFF;
		this->raw |= (_Address << 12);
	}

	__always_inline inline nsa nif uintptr_t GetAddress() { return (this->raw & 0x000FFFFFFFFFF000) >> 12; }
};

struct __attribute__((packed)) PageDirectoryEntryPtr
{
	PageDirectoryEntry Entries[512];
};

union __attribute__((packed)) PageDirectoryPointerTableEntry
{
	struct
	{
		bool Present : 1;         // 0
		bool ReadWrite : 1;       // 1
		bool UserSupervisor : 1;  // 2
		bool WriteThrough : 1;    // 3
		bool CacheDisable : 1;    // 4
		bool Accessed : 1;        // 5
		bool Available0 : 1;      // 6
		bool PageSize : 1;        // 7
		uint8_t Available1 : 4;   // 8-11
		uint64_t Address : 40;    // 12-51
		uint32_t Available2 : 11; // 52-62
		bool ExecuteDisable : 1;  // 63
	};
	uint64_t raw;

	__always_inline inline nsa nif void SetAddress(uintptr_t _Address)
	{
		_Address &= 0x000000FFFFFFFFFF;
		this->raw &= 0xFFF0000000000FFF;
		this->raw |= (_Address << 12);
	}

	__always_inline inline nsa nif uintptr_t GetAddress() { return (this->raw & 0x000FFFFFFFFFF000) >> 12; }
};

struct __attribute__((packed)) PageDirectoryPointerTableEntryPtr
{
	PageDirectoryPointerTableEntry Entries[512];
};

union __attribute__((packed)) PageMapLevel4
{
	struct
	{
		bool Present : 1;         // 0
		bool ReadWrite : 1;       // 1
		bool UserSupervisor : 1;  // 2
		bool WriteThrough : 1;    // 3
		bool CacheDisable : 1;    // 4
		bool Accessed : 1;        // 5
		bool Available0 : 1;      // 6
		bool Reserved0 : 1;       // 7
		uint8_t Available1 : 4;   // 8-11
		uint64_t Address : 40;    // 12-51
		uint32_t Available2 : 11; // 52-62
		bool ExecuteDisable : 1;  // 63
	};
	uint64_t raw;

	__always_inline inline nsa nif void SetAddress(uintptr_t _Address)
	{
		_Address &= 0x000000FFFFFFFFFF;
		this->raw &= 0xFFF0000000000FFF;
		this->raw |= (_Address << 12);
	}

	__always_inline inline nsa nif uintptr_t GetAddress() { return (this->raw & 0x000FFFFFFFFFF000) >> 12; }
};

struct PageTable4
{
	PageMapLevel4 Entries[512];
} __attribute__((aligned(0x1000)));

extern "C" char BootPageTable[];
extern uintptr_t _kernel_start, _kernel_end;

__attribute__((section(".bootstrap.data"))) static PageTable4 *BPTable = (PageTable4 *)BootPageTable;
__attribute__((section(".bootstrap.data"))) static size_t BPT_Allocated = 0x4000;

__always_inline inline nsa nif void *RequestPage()
{
	void *Page = (void *)(BootPageTable + BPT_Allocated);
	BPT_Allocated += 0x1000;
	if (BPT_Allocated >= 0x10000) /* The length of BootPageTable */
	{
		while (true)
			;
	}
	return Page;
}

class PageMapIndexer
{
public:
	uintptr_t PMLIndex = 0;
	uintptr_t PDPTEIndex = 0;
	uintptr_t PDEIndex = 0;
	uintptr_t PTEIndex = 0;
	__always_inline inline nsa nif PageMapIndexer(uintptr_t VirtualAddress)
	{
		uintptr_t Address = VirtualAddress;
		Address >>= 12;
		this->PTEIndex = Address & 0x1FF;
		Address >>= 9;
		this->PDEIndex = Address & 0x1FF;
		Address >>= 9;
		this->PDPTEIndex = Address & 0x1FF;
		Address >>= 9;
		this->PMLIndex = Address & 0x1FF;
	}
};

__attribute__((section(".bootstrap.text"))) nsa nif void MB2_64_Map(void *VirtualAddress, void *PhysicalAddress, uint64_t Flags)
{
	PageMapIndexer Index = PageMapIndexer((uintptr_t)VirtualAddress);
	// Clear any flags that are not 1 << 0 (Present) - 1 << 5 (Accessed) because rest are for page table entries only
	uint64_t DirectoryFlags = Flags & 0x3F;

	PageMapLevel4 PML4 = BPTable->Entries[Index.PMLIndex];
	PageDirectoryPointerTableEntryPtr *PDPTEPtr = nullptr;
	if (!PML4.Present)
	{
		PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)RequestPage();
		if (PDPTEPtr == nullptr)
			return;
		{
			void *ptr = PDPTEPtr;
			uint8_t value = 0;
			size_t num = 0x1000;
			uint8_t *p = (uint8_t *)ptr;
			for (size_t i = 0; i < num; i++)
				p[i] = value;
		}
		PML4.Present = true;
		PML4.SetAddress((uintptr_t)PDPTEPtr >> 12);
	}
	else
		PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)((uintptr_t)PML4.GetAddress() << 12);
	PML4.raw |= DirectoryFlags;
	BPTable->Entries[Index.PMLIndex] = PML4;

	PageDirectoryPointerTableEntry PDPTE = PDPTEPtr->Entries[Index.PDPTEIndex];
	PageDirectoryEntryPtr *PDEPtr = nullptr;
	if (!PDPTE.Present)
	{
		PDEPtr = (PageDirectoryEntryPtr *)RequestPage();
		if (PDEPtr == nullptr)
			return;
		{
			void *ptr = PDEPtr;
			uint8_t value = 0;
			size_t num = 0x1000;
			uint8_t *p = (uint8_t *)ptr;
			for (size_t i = 0; i < num; i++)
				p[i] = value;
		}
		PDPTE.Present = true;
		PDPTE.SetAddress((uintptr_t)PDEPtr >> 12);
	}
	else
		PDEPtr = (PageDirectoryEntryPtr *)((uintptr_t)PDPTE.GetAddress() << 12);
	PDPTE.raw |= DirectoryFlags;
	PDPTEPtr->Entries[Index.PDPTEIndex] = PDPTE;

	PageDirectoryEntry PDE = PDEPtr->Entries[Index.PDEIndex];
	PageTableEntryPtr *PTEPtr = nullptr;
	if (!PDE.Present)
	{
		PTEPtr = (PageTableEntryPtr *)RequestPage();
		if (PTEPtr == nullptr)
			return;
		{
			void *ptr = PTEPtr;
			uint8_t value = 0;
			size_t num = 0x1000;
			uint8_t *p = (uint8_t *)ptr;
			for (size_t i = 0; i < num; i++)
				p[i] = value;
		}
		PDE.Present = true;
		PDE.SetAddress((uintptr_t)PTEPtr >> 12);
	}
	else
		PTEPtr = (PageTableEntryPtr *)((uintptr_t)PDE.GetAddress() << 12);
	PDE.raw |= DirectoryFlags;
	PDEPtr->Entries[Index.PDEIndex] = PDE;

	PageTableEntry PTE = PTEPtr->Entries[Index.PTEIndex];
	PTE.Present = true;
	PTE.raw |= Flags;
	PTE.SetAddress((uintptr_t)PhysicalAddress >> 12);
	PTEPtr->Entries[Index.PTEIndex] = PTE;
	asmv("invlpg (%0)"
		 :
		 : "r"(VirtualAddress)
		 : "memory");
}

EXTERNC __attribute__((section(".bootstrap.text"))) nsa nif __attribute__((section(".bootstrap.text"))) void UpdatePageTable64()
{
	BPTable = (PageTable4 *)BootPageTable;

	uintptr_t KernelStart = (uintptr_t)&_kernel_start;
	uintptr_t KernelEnd = (uintptr_t)&_kernel_end;
	uintptr_t PhysicalStart = KernelStart - 0xFFFFFFFF80000000;
	for (uintptr_t i = KernelStart; i < KernelEnd; i += 0x1000)
	{
		MB2_64_Map((void *)i, (void *)PhysicalStart, 0x3);
		PhysicalStart += 0x1000;
	}

	asmv("mov %%cr3, %%rax\n"
		 "mov %%rax, %%cr3\n"
		 :
		 :
		 : "rax");
}
