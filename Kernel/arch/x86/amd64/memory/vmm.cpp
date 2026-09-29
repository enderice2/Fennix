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
	bool Virtual::Check(fnx::void_t VirtualAddress, PTFlag Flag, MapType Type)
	{
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];

		PageDirectoryPointerTableEntryPtr *pdpte = nullptr;
		PageDirectoryEntryPtr *pde = nullptr;
		PageTableEntryPtr *pte = nullptr;

		if (!pml4->Present)
		{
			debug("PML4 not present for %#lx", VirtualAddress.get());
			return false;
		}

		pdpte = pml4->GetPDPTE();
		if (!pdpte)
		{
			debug("Failed to get PDPTE for %#lx", VirtualAddress.get());
			return false;
		}

		if (pdpte->Entries[pmi.PDPTEIndex].PageSize)
		{
			bool result = pdpte->Entries[pmi.PDPTEIndex].raw & Flag;
			if (!result)
			{
				debug("Failed to check %#lx for %#lx (raw: %#lx)", VirtualAddress.get(), Flag,
					  pdpte->Entries[pmi.PDPTEIndex].raw);
			}
			return result;
		}

		pde = pdpte->Entries[pmi.PDPTEIndex].GetPDE();
		if (!pde)
		{
			debug("Failed to get PDE for %#lx", VirtualAddress.get());
			return false;
		}

		if (pde->Entries[pmi.PDEIndex].PageSize)
		{
			bool result = pde->Entries[pmi.PDEIndex].raw & Flag;
			if (!result)
			{
				debug("Failed to check %#lx for %#lx (raw: %#lx)", VirtualAddress.get(), Flag,
					  pde->Entries[pmi.PDEIndex].raw);
			}
			return result;
		}

		pte = pde->Entries[pmi.PDEIndex].GetPTE();
		if (!pte)
		{
			debug("Failed to get PTE for %#lx", VirtualAddress.get());
			return false;
		}

		bool result = pte->Entries[pmi.PTEIndex].raw & Flag;
		if (!result)
		{
			debug("Failed to check %#lx for %#lx (raw: %#lx)", VirtualAddress.get(), Flag,
				  pte->Entries[pmi.PTEIndex].raw);
		}
		return result;
	}

	fnx::void_t Virtual::GetPhysical(fnx::void_t VirtualAddress)
	{
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];

		PageDirectoryPointerTableEntryPtr *pdpte = nullptr;
		PageDirectoryEntryPtr *pde = nullptr;
		PageTableEntryPtr *pte = nullptr;

		if (!pml4->Present)
			return nullptr;

		pdpte = pml4->GetPDPTE();
		if (!pdpte)
			return nullptr;

		if (!pdpte->Entries[pmi.PDPTEIndex].Present)
			return nullptr;

		if (pdpte->Entries[pmi.PDPTEIndex].PageSize)
			return pdpte->Entries[pmi.PDPTEIndex].GetAddress() << 12;

		pde = pdpte->Entries[pmi.PDPTEIndex].GetPDE();
		if (!pde)
			return nullptr;

		if (!pde->Entries[pmi.PDEIndex].Present)
			return nullptr;

		if (pde->Entries[pmi.PDEIndex].PageSize)
			return pde->Entries[pmi.PDEIndex].GetAddress() << 12;

		pte = pde->Entries[pmi.PDEIndex].GetPTE();
		if (!pte)
			return nullptr;

		if (pte->Entries[pmi.PTEIndex].Present)
			return pte->Entries[pmi.PTEIndex].GetAddress() << 12;

		return nullptr;
	}

	Virtual::MapType Virtual::GetMapType(fnx::void_t VirtualAddress)
	{
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];

		PageDirectoryPointerTableEntryPtr *pdpte = nullptr;
		PageDirectoryEntryPtr *pde = nullptr;
		PageTableEntryPtr *pte = nullptr;

		if (!pml4->Present)
			goto ReturnLogError;

		pdpte = pml4->GetPDPTE();
		if (!pdpte || !pdpte->Entries[pmi.PDPTEIndex].Present)
			goto ReturnLogError;

		if (pdpte->Entries[pmi.PDPTEIndex].PageSize)
			return MapType::OneGiB;

		pde = pdpte->Entries[pmi.PDPTEIndex].GetPDE();
		if (!pde || !pde->Entries[pmi.PDEIndex].Present)
			goto ReturnLogError;

		if (pde->Entries[pmi.PDEIndex].PageSize)
			return MapType::TwoMiB;

		pte = pde->Entries[pmi.PDEIndex].GetPTE();
		if (!pte)
			goto ReturnLogError;

		if (pte->Entries[pmi.PTEIndex].Present)
			return MapType::FourKiB;

	ReturnLogError:
		return MapType::NoMapType;
	}

	PageMapLevel5 *Virtual::GetPML5(fnx::void_t VirtualAddress, MapType Type)
	{
		UNUSED(VirtualAddress);
		UNUSED(Type);
		stub; /* TODO */
		return nullptr;
	}

	PageMapLevel4 *Virtual::GetPML4(fnx::void_t VirtualAddress, MapType Type)
	{
		UNUSED(Type);
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];
		if (pml4->Present)
			return pml4;

		debug("PML4 not present for %#lx", VirtualAddress.get());
		return nullptr;
	}

	PageDirectoryPointerTableEntry *Virtual::GetPDPTE(fnx::void_t VirtualAddress, MapType Type)
	{
		UNUSED(Type);
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];
		if (!pml4->Present)
		{
			debug("PML4 not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageDirectoryPointerTableEntryPtr *PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)((uintptr_t)pml4->Address << 12);
		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (PDPTE->Present)
			return PDPTE;

		debug("PDPTE not present for %#lx", VirtualAddress.get());
		return nullptr;
	}

	PageDirectoryEntry *Virtual::GetPDE(fnx::void_t VirtualAddress, MapType Type)
	{
		UNUSED(Type);
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *PML4 = &this->pTable->Entries[pmi.PMLIndex];
		if (!PML4->Present)
		{
			debug("PML4 not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageDirectoryPointerTableEntryPtr *PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)((uintptr_t)PML4->Address << 12);
		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (!PDPTE->Present)
		{
			debug("PDPTE not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageDirectoryEntryPtr *PDEPtr = (PageDirectoryEntryPtr *)(PDPTE->GetAddress() << 12);
		PageDirectoryEntry *PDE = &PDEPtr->Entries[pmi.PDEIndex];
		if (PDE->Present)
			return PDE;

		debug("PDE not present for %#lx", VirtualAddress.get());
		return nullptr;
	}

	PageTableEntry *Virtual::GetPTE(fnx::void_t VirtualAddress, MapType Type)
	{
		UNUSED(Type);
		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *PML4 = &this->pTable->Entries[pmi.PMLIndex];
		if (!PML4->Present)
		{
			debug("PML4 not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageDirectoryPointerTableEntryPtr *PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)((uintptr_t)PML4->Address << 12);
		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (!PDPTE->Present)
		{
			debug("PDPTE not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageDirectoryEntryPtr *PDEPtr = (PageDirectoryEntryPtr *)(PDPTE->GetAddress() << 12);
		PageDirectoryEntry *PDE = &PDEPtr->Entries[pmi.PDEIndex];
		if (!PDE->Present)
		{
			debug("PDE not present for %#lx", VirtualAddress.get());
			return nullptr;
		}

		PageTableEntryPtr *PTEPtr = (PageTableEntryPtr *)(PDE->GetAddress() << 12);
		PageTableEntry *PTE = &PTEPtr->Entries[pmi.PTEIndex];
		if (PTE->Present)
			return PTE;

		debug("PTE not present for %#lx", VirtualAddress.get());
		return nullptr;
	}

	hot void Virtual::SingleMap(fnx::void_t VirtualAddress, fnx::void_t PhysicalAddress, uint64_t Flags, MapType Type)
	{
		SmartLock(this->MemoryLock);
		if (unlikely(!this->pTable))
		{
			error("No page table");
			return;
		}

		constexpr uint64_t LEAF_FLAG_MASK =
			PTFlag::P |
			PTFlag::RW |
			PTFlag::US |
			PTFlag::XD |
			PTFlag::PWT |
			PTFlag::PCD |
			PTFlag::G;

		auto split2M = [&](PageDirectoryEntry *pde)
		{
			if (!pde->Present || !pde->PageSize)
				return;

			uintptr_t base = pde->GetAddress() << 12;
			uintptr_t flags = pde->raw;

			auto newPT = (PageTableEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageTableEntryPtr) + 1));
			memset(newPT, 0, PAGE_SIZE_4K);

			for (size_t i = 0; i < numof(newPT->Entries); i++)
			{
				PageTableEntry &e = newPT->Entries[i];
				e.Present = true;
				e.raw |= (flags & LEAF_FLAG_MASK);
				e.SetAddress((base + i * PAGE_SIZE_4K) >> 12);
			}

			pde->PageSize = false;
			pde->SetAddress((uintptr_t)newPT >> 12);
		};

		auto split1G = [&](PageDirectoryPointerTableEntry *pdpte)
		{
			if (!pdpte->Present || !pdpte->PageSize)
				return;

			uintptr_t base = pdpte->GetAddress() << 12;
			uintptr_t flags = pdpte->raw;

			auto newPD = (PageDirectoryEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageDirectoryEntryPtr) + 1));
			memset(newPD, 0, PAGE_SIZE_4K);

			for (size_t i = 0; i < numof(newPD->Entries); i++)
			{
				auto &e = newPD->Entries[i];
				e.Present = true;
				e.PageSize = true;
				e.raw |= (flags & LEAF_FLAG_MASK);
				e.SetAddress((base + i * PAGE_SIZE_2M) >> 12);
			}

			pdpte->PageSize = false;
			pdpte->SetAddress((uintptr_t)newPD >> 12);
		};

		auto collapse2M = [&](PageDirectoryEntry *pde)
		{
			if (unlikely(!pde->Present || pde->PageSize))
				return;

			auto pt = pde->GetPTE();
			KernelAllocator.FreePages(pt, TO_PAGES(sizeof(PageTableEntryPtr) + 1));

			pde->raw = 0;
			pde->PageSize = true;
		};

		auto collapse1G = [&](PageDirectoryPointerTableEntry *pdpte)
		{
			if (unlikely(!pdpte->Present || pdpte->PageSize))
				return;

			auto pd = pdpte->GetPDE();

			for (size_t i = 0; i < numof(pd->Entries); i++)
			{
				PageDirectoryEntry &pde = pd->Entries[i];
				if (!pde.Present)
					continue;

				if (!pde.PageSize)
					collapse2M(&pde);
			}

			KernelAllocator.FreePages(pd, TO_PAGES(sizeof(PageDirectoryEntryPtr) + 1));

			pdpte->raw = 0;
			pdpte->PageSize = true;
		};

		Flags |= PTFlag::P;
		// Clear any flags that are not 1 << 0 (Present) - 1 << 5 (Accessed) because rest are for page table entries only
		uint64_t dirFlags = Flags & 0x3F;

		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *pml4 = &this->pTable->Entries[pmi.PMLIndex];

		PageDirectoryPointerTableEntryPtr *PDPTEPtr = nullptr;
		if (!pml4->Present)
		{
			PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageDirectoryPointerTableEntryPtr) + 1));
			memset(PDPTEPtr, 0, sizeof(PageDirectoryPointerTableEntryPtr));
			pml4->Present = true;
			pml4->SetAddress((uintptr_t)PDPTEPtr >> 12);
		}
		else
			PDPTEPtr = pml4->GetPDPTE();
		pml4->raw |= dirFlags;

		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (Type == OneGiB)
		{
			if (!PDPTE->PageSize)
				collapse1G(PDPTE);

			PDPTE->raw |= Flags;
			PDPTE->PageSize = true;
			PDPTE->SetAddress((uintptr_t)PhysicalAddress >> 12);
			// debug("Mapped 1GB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
			return;
		}

		if (PDPTE->PageSize)
			split1G(PDPTE);

		PageDirectoryEntryPtr *PDEPtr = nullptr;
		if (!PDPTE->Present)
		{
			PDEPtr = (PageDirectoryEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageDirectoryEntryPtr) + 1));
			memset(PDEPtr, 0, sizeof(PageDirectoryEntryPtr));
			PDPTE->Present = true;
			PDPTE->SetAddress((uintptr_t)PDEPtr >> 12);
		}
		else
			PDEPtr = (PageDirectoryEntryPtr *)(PDPTE->GetAddress() << 12);
		PDPTE->raw |= dirFlags;

		PageDirectoryEntry *PDE = &PDEPtr->Entries[pmi.PDEIndex];
		if (Type == TwoMiB)
		{
			if (!PDE->PageSize)
				collapse2M(PDE);

			PDE->raw |= Flags;
			PDE->PageSize = true;
			PDE->SetAddress((uintptr_t)PhysicalAddress >> 12);
			// debug("Mapped 2MB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
			return;
		}

		if (PDE->PageSize)
			split2M(PDE);

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
		PDE->raw |= dirFlags;

		PageTableEntry *PTE = &PTEPtr->Entries[pmi.PTEIndex];
		PTE->Present = true;
		PTE->raw |= Flags;
		PTE->SetAddress((uintptr_t)PhysicalAddress >> 12);
		CPU::x64::invlpg(VirtualAddress);
		/* FIXME: CRITICAL: TLB shootdown on other cores */

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

		if (!this->Check(VirtualAddress, (PTFlag)Flags)) // quick workaround just to see where it fails
			warn("Failed to map v:%#lx p:%#lx with flags: " BYTE_TO_BINARY_PATTERN, VirtualAddress.get(), PhysicalAddress.get(), BYTE_TO_BINARY(Flags));
#endif
	}

	void Virtual::OptimizedMap(fnx::void_t VirtualAddress, fnx::void_t PhysicalAddress, size_t Length, uint64_t Flags)
	{
		static bool support1GB = false;

		static int once = 0;
		if (!once++) /* this is thread-safe because it's called in early-boot */
		{
			if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
			{
				CPU::x86::AMD::CPUID0x80000001 cpuid;
				support1GB = cpuid.EDX.Page1GB;
			}
			else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
			{
				CPU::x86::AMD::CPUID0x80000001 cpuid;
				support1GB = cpuid.EDX.Page1GB;
				/* FIXME: use the proper CPUID struct for Intel CPUs. */
			}

			if (support1GB)
			{
				debug("1GB Page Support Enabled");
			}
		}

		/* We gettin greedy with this one */
		while (Length >= PAGE_SIZE_4K)
		{
			bool fit1G = Length >= PAGE_SIZE_1G;
			bool fit2M = Length >= PAGE_SIZE_2M;
			bool aligned1G = is_aligned(VirtualAddress, PAGE_SIZE_1G) && is_aligned(PhysicalAddress, PAGE_SIZE_1G);
			bool aligned2M = is_aligned(VirtualAddress, PAGE_SIZE_2M) && is_aligned(PhysicalAddress, PAGE_SIZE_2M);

			if (support1GB && fit1G && aligned1G)
			{
				// debug("Optimized mapping 1GB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
				this->SingleMap(VirtualAddress, PhysicalAddress, Flags, Virtual::MapType::OneGiB);
				VirtualAddress += PAGE_SIZE_1G;
				PhysicalAddress += PAGE_SIZE_1G;
				Length -= PAGE_SIZE_1G;
			}
			else if (fit2M && aligned2M)
			{
				// debug("Optimized mapping 2MB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
				this->SingleMap(VirtualAddress, PhysicalAddress, Flags, Virtual::MapType::TwoMiB);
				VirtualAddress += PAGE_SIZE_2M;
				PhysicalAddress += PAGE_SIZE_2M;
				Length -= PAGE_SIZE_2M;
			}
			else
			{
				// debug("Mapping 4KB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
				this->SingleMap(VirtualAddress, PhysicalAddress, Flags, Virtual::MapType::FourKiB);
				VirtualAddress += PAGE_SIZE_4K;
				PhysicalAddress += PAGE_SIZE_4K;
				Length -= PAGE_SIZE_4K;
			}
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

		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		PageMapLevel4 *PML4 = &this->pTable->Entries[pmi.PMLIndex];
		if (!PML4->Present)
			return;

		PageDirectoryPointerTableEntryPtr *PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)((uintptr_t)PML4->Address << 12);
		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (!PDPTE->Present)
			return;

		if (Type == MapType::OneGiB && PDPTE->PageSize)
		{
			PDPTE->Present = false;
			return;
		}

		PageDirectoryEntryPtr *PDEPtr = (PageDirectoryEntryPtr *)((uintptr_t)PDPTE->Address << 12);
		PageDirectoryEntry *PDE = &PDEPtr->Entries[pmi.PDEIndex];
		if (!PDE->Present)
			return;

		if (Type == MapType::TwoMiB && PDE->PageSize)
		{
			PDE->Present = false;
			return;
		}

		PageTableEntryPtr *PTEPtr = (PageTableEntryPtr *)((uintptr_t)PDE->Address << 12);
		PageTableEntry PTE = PTEPtr->Entries[pmi.PTEIndex];
		if (!PTE.Present)
			return;

		PTE.Present = false;
		PTEPtr->Entries[pmi.PTEIndex] = PTE;
		CPU::x64::invlpg(VirtualAddress);
	}

	void Virtual::Remap(fnx::void_t VirtualAddress, fnx::void_t PhysicalAddress, uint64_t Flags, MapType Type)
	{
		SmartLock(this->MemoryLock);
		if (unlikely(!this->pTable))
		{
			error("No page table");
			return;
		}

		Flags |= PTFlag::P;

		PageMapIndexer pmi = PageMapIndexer(VirtualAddress);
		// Clear any flags that are not 1 << 0 (Present) - 1 << 5 (Accessed) because rest are for page table entries only
		uint64_t dirFlags = Flags & 0x3F;

		PageMapLevel4 *PML4 = &this->pTable->Entries[pmi.PMLIndex];
		PageDirectoryPointerTableEntryPtr *PDPTEPtr = nullptr;
		if (!PML4->Present)
		{
			PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageDirectoryPointerTableEntryPtr) + 1));
			memset(PDPTEPtr, 0, sizeof(PageDirectoryPointerTableEntryPtr));
			PML4->Present = true;
			PML4->SetAddress((uintptr_t)PDPTEPtr >> 12);
		}
		else
			PDPTEPtr = (PageDirectoryPointerTableEntryPtr *)(PML4->GetAddress() << 12);
		PML4->raw |= dirFlags;

		PageDirectoryPointerTableEntry *PDPTE = &PDPTEPtr->Entries[pmi.PDPTEIndex];
		if (Type == MapType::OneGiB)
		{
			PDPTE->raw &= 0xFFF;
			PDPTE->raw |= Flags;
			PDPTE->PageSize = true;
			PDPTE->SetAddress((uintptr_t)PhysicalAddress >> 12);
			debug("Mapped 1GB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
			return;
		}

		PageDirectoryEntryPtr *PDEPtr = nullptr;
		if (!PDPTE->Present)
		{
			PDEPtr = (PageDirectoryEntryPtr *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageDirectoryEntryPtr) + 1));
			memset(PDEPtr, 0, sizeof(PageDirectoryEntryPtr));
			PDPTE->Present = true;
			PDPTE->SetAddress((uintptr_t)PDEPtr >> 12);
		}
		else
			PDEPtr = (PageDirectoryEntryPtr *)(PDPTE->GetAddress() << 12);
		PDPTE->raw |= dirFlags;

		PageDirectoryEntry *PDE = &PDEPtr->Entries[pmi.PDEIndex];
		if (Type == MapType::TwoMiB)
		{
			PDE->raw &= 0xFFF;
			PDE->raw |= Flags;
			PDE->PageSize = true;
			PDE->SetAddress((uintptr_t)PhysicalAddress >> 12);
			debug("Mapped 2MB page at %#lx to %#lx", VirtualAddress.get(), PhysicalAddress.get());
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
		PDE->raw |= dirFlags;

		PageTableEntry *PTE = &PTEPtr->Entries[pmi.PTEIndex];
		PTE->raw &= 0xFFF;
		PTE->raw |= Flags;
		PTE->Present = true;
		PTE->SetAddress((uintptr_t)PhysicalAddress >> 12);
		CPU::x64::invlpg(VirtualAddress);
	}
}
