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
#include <lock.hpp>
#include <debug.h>
#ifdef DEBUG
#include <uart.hpp>
#endif

#include "heap_allocators/liballoc_1_1/liballoc_1_1.h"
#include "heap_allocators/rpmalloc/rpmalloc.h"
#include "../../kernel.h"

// #define DEBUG_ALLOCATIONS 1

#ifdef DEBUG_ALLOCATIONS
#define memdbg(m, ...)       \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define memdbg(m, ...)
#endif

using namespace Memory;

Physical KernelAllocator;
KernelStackManager StackManager;
DMA dma;
PageTable *KernelPageTable = nullptr;

MemoryAllocatorType AllocatorType = MemoryAllocatorType::liballoc11;

nif static void MapEntries(PageTable *PT)
{
	debug("mapping %d memory entries", bInfo.Memory.Entries);
	Virtual vmm = Virtual(PT);

	for (uint64_t i = 0; i < bInfo.Memory.Entries; i++)
	{
		fnx::ptr_t Base = bInfo.Memory.Entry[i].BaseAddress;
		size_t Length = bInfo.Memory.Entry[i].Length;

		debug("mapping %#lx-%#lx", Base.get(), Base.as<uintptr_t>() + Length);
		vmm.OptimizedMap(Base, Base, Length, RW);
	}

	/* Make sure 0x0 is unmapped (so we PF when nullptr is accessed) */
	vmm.Unmap((void *)0);
}

nif static void MapFramebuffer(PageTable *PT)
{
	debug("Mapping Framebuffer");
	Virtual vmm = Virtual(PT);
	int itrfb = 0;
	while (1)
	{
		if (!bInfo.Framebuffer[itrfb].BaseAddress)
			break;

		size_t fbSize = bInfo.Framebuffer[itrfb].Pitch * bInfo.Framebuffer[itrfb].Height;
		fbSize = ALIGN_UP(fbSize, PAGE_SIZE);

#ifdef DEBUG
		if (DebuggerIsAttached)
			fbSize += 16 * PAGE_SIZE;
#endif

		vmm.OptimizedMap(bInfo.Framebuffer[itrfb].BaseAddress,
						 bInfo.Framebuffer[itrfb].BaseAddress,
						 fbSize, RW | PWT | G | KRsv);

		itrfb++;
	}
}

nif static void MapKernel(PageTable *PT)
{
	debug("Mapping Kernel");

	/* RWX */
	uintptr_t BootstrapStart = (uintptr_t)&_bootstrap_start;
	uintptr_t BootstrapEnd = (uintptr_t)&_bootstrap_end;

	/* RX */
	uintptr_t KernelTextStart = (uintptr_t)&_kernel_text_start;
	uintptr_t KernelTextEnd = (uintptr_t)&_kernel_text_end;

	/* RW */
	uintptr_t KernelDataStart = (uintptr_t)&_kernel_data_start;
	uintptr_t KernelDataEnd = (uintptr_t)&_kernel_data_end;

	/* R */
	uintptr_t KernelRoDataStart = (uintptr_t)&_kernel_rodata_start;
	uintptr_t KernelRoDataEnd = (uintptr_t)&_kernel_rodata_end;

	/* RW */
	uintptr_t KernelBssStart = (uintptr_t)&_kernel_bss_start;
	uintptr_t KernelBssEnd = (uintptr_t)&_kernel_bss_end;

#ifdef DEBUG
	uintptr_t KernelStart = (uintptr_t)&_kernel_start;
	uintptr_t KernelEnd = (uintptr_t)&_kernel_end;
#endif
	uintptr_t KernelFileStart = (uintptr_t)bInfo.Kernel.FileBase;
	uintptr_t KernelFileEnd = KernelFileStart + bInfo.Kernel.Size;

	debug("Bootstrap: %#lx-%#lx", BootstrapStart, BootstrapEnd);
	debug("Kernel text: %#lx-%#lx", KernelTextStart, KernelTextEnd);
	debug("Kernel data: %#lx-%#lx", KernelDataStart, KernelDataEnd);
	debug("Kernel rodata: %#lx-%#lx", KernelRoDataStart, KernelRoDataEnd);
	debug("Kernel bss: %#lx-%#lx", KernelBssStart, KernelBssEnd);
	debug("Kernel: %#lx-%#lx", KernelStart, KernelEnd);
	debug("Kernel file: %#lx-%#lx", KernelFileStart, KernelFileEnd);

	debug("File size: %ld KiB", TO_KiB(bInfo.Kernel.Size));
	debug(".bootstrap size: %ld KiB", TO_KiB(BootstrapEnd - BootstrapStart));
	debug(".text size: %ld KiB", TO_KiB(KernelTextEnd - KernelTextStart));
	debug(".data size: %ld KiB", TO_KiB(KernelDataEnd - KernelDataStart));
	debug(".rodata size: %ld KiB", TO_KiB(KernelRoDataEnd - KernelRoDataStart));
	debug(".bss size: %ld KiB", TO_KiB(KernelBssEnd - KernelBssStart));

	uintptr_t BaseKernelMapAddress = (uintptr_t)bInfo.Kernel.PhysicalBase;
	debug("Base kernel map address: %#lx", BaseKernelMapAddress);
	uintptr_t k;
	Virtual vmm = Virtual(PT);

	/* Bootstrap section */
	if (BaseKernelMapAddress == BootstrapStart)
	{
		vmm.OptimizedMap(BootstrapStart, BaseKernelMapAddress, BootstrapEnd - BootstrapStart, RW | G | KRsv);
		KernelAllocator.ReservePages((void *)BaseKernelMapAddress, TO_PAGES(BootstrapEnd - BootstrapStart));
		for (k = BootstrapStart; k < BootstrapEnd; k += PAGE_SIZE)
			BaseKernelMapAddress += PAGE_SIZE;
	}
	else
	{
		trace("Ignoring bootstrap section.");
		/* Bootstrap section must be mapped at 0x100000. */
	}

	/* Text section */
	vmm.OptimizedMap(KernelTextStart, BaseKernelMapAddress, KernelTextEnd - KernelTextStart, RW | G | KRsv); /* FIXME: i must remove RW flag here */
	KernelAllocator.ReservePages((void *)BaseKernelMapAddress, TO_PAGES(KernelTextEnd - KernelTextStart));
	for (k = KernelTextStart; k < KernelTextEnd; k += PAGE_SIZE)
		BaseKernelMapAddress += PAGE_SIZE;

	/* Data section */
	vmm.OptimizedMap(KernelDataStart, BaseKernelMapAddress, KernelDataEnd - KernelDataStart, RW | G | KRsv);
	KernelAllocator.ReservePages((void *)BaseKernelMapAddress, TO_PAGES(KernelDataEnd - KernelDataStart));
	for (k = KernelDataStart; k < KernelDataEnd; k += PAGE_SIZE)
		BaseKernelMapAddress += PAGE_SIZE;

	/* Read only data section */
	vmm.OptimizedMap(KernelRoDataStart, BaseKernelMapAddress, KernelRoDataEnd - KernelRoDataStart, G | KRsv);
	KernelAllocator.ReservePages((void *)BaseKernelMapAddress, TO_PAGES(KernelRoDataEnd - KernelRoDataStart));
	for (k = KernelRoDataStart; k < KernelRoDataEnd; k += PAGE_SIZE)
		BaseKernelMapAddress += PAGE_SIZE;

	/* Block starting symbol section */
	vmm.OptimizedMap(KernelBssStart, BaseKernelMapAddress, KernelBssEnd - KernelBssStart, RW | G | KRsv);
	KernelAllocator.ReservePages((void *)BaseKernelMapAddress, TO_PAGES(KernelBssEnd - KernelBssStart));
	for (k = KernelBssStart; k < KernelBssEnd; k += PAGE_SIZE)
		BaseKernelMapAddress += PAGE_SIZE;

	debug("Base kernel map address: %#lx", BaseKernelMapAddress);

	/* Kernel file */
	if (KernelFileStart != 0)
	{
		vmm.OptimizedMap(KernelFileStart, KernelFileStart, KernelFileEnd - KernelFileStart, G | KRsv);
		KernelAllocator.ReservePages((void *)KernelFileStart, TO_PAGES(KernelFileEnd - KernelFileStart));
	}
	else
		info("Cannot determine kernel file address. Ignoring.");
}

namespace Memory
{
	nif void CreatePageTable(PageTable *pt)
	{
		MapEntries(pt);
		MapFramebuffer(pt);
		MapKernel(pt);
	}

	nif void Initialize()
	{
#ifdef DEBUG
#ifndef __i386__
		for (uint64_t i = 0; i < bInfo.Memory.Entries; i++)
		{
			uintptr_t Base = reinterpret_cast<uintptr_t>(bInfo.Memory.Entry[i].BaseAddress);
			size_t Length = bInfo.Memory.Entry[i].Length;
			uintptr_t End = Base + Length;
			const char *Type = "Unknown";

			switch (bInfo.Memory.Entry[i].Type)
			{
			case likely(Usable):
				Type = "Usable";
				break;
			case Reserved:
				Type = "Reserved";
				break;
			case ACPIReclaimable:
				Type = "ACPI Reclaimable";
				break;
			case ACPINVS:
				Type = "ACPI NVS";
				break;
			case BadMemory:
				Type = "Bad Memory";
				break;
			case BootloaderReclaimable:
				Type = "Bootloader Reclaimable";
				break;
			case KernelAndModules:
				Type = "Kernel and Modules";
				break;
			case Framebuffer:
				Type = "Framebuffer";
				break;
			default:
				break;
			}

			debug("%02ld: %p-%p %s",
				  i,
				  Base,
				  End,
				  Type);
		}
#endif // __i386__
#endif // DEBUG
		trace("Initializing Physical Memory Manager");
		// KernelAllocator = Physical(); <- Already called in the constructor
		KernelAllocator.Init();
		debug("Memory Info:\n\n%lld MiB / %lld MiB (%lld MiB reserved)\n",
			  TO_MiB(KernelAllocator.GetUsedMemory()),
			  TO_MiB(KernelAllocator.GetTotalMemory() - KernelAllocator.GetReservedMemory()),
			  TO_MiB(KernelAllocator.GetReservedMemory()));

		/* -- Debugging --
			size_t bmap_size = KernelAllocator.GetPageBitmap().Size;
			for (size_t i = 0; i < bmap_size; i++)
			{
				bool idx = KernelAllocator.GetPageBitmap().Get(i);
				if (idx == true)
					debug("Page %04d: %#lx", i, i * PAGE_SIZE);
			}

			inf_loop debug("Alloc.: %#lx", KernelAllocator.RequestPage());
		*/

		trace("Initializing Virtual Memory Manager");
		KernelPageTable = (PageTable *)KernelAllocator.RequestPages(TO_PAGES(sizeof(PageTable)) + 1);
		memset(KernelPageTable, 0, PAGE_SIZE);

		CreatePageTable(KernelPageTable);

		trace("Applying new page table from address %#lx", KernelPageTable);
		CPU::PageTable(KernelPageTable);
		debug("Page table updated");

		/* FIXME: Read kernel params */
		AllocatorType = Config.AllocatorType;

		switch (AllocatorType)
		{
		case MemoryAllocatorType::liballoc11:
			break;
		case MemoryAllocatorType::rpmalloc_:
		{
			rpmalloc_initialize(0);
			// rpmalloc_config_t config = {
			// 	.memory_map = nullptr,
			// 	.memory_unmap = nullptr,
			// 	.error_callback = nullptr,
			// 	.map_fail_callback = nullptr,
			// 	.page_size = PAGE_SIZE,
			// 	.span_size = 4 * 1024, /* 4 KiB */
			// 	.span_map_count = 1,
			// 	.enable_huge_pages = 0,
			// 	.page_name = nullptr,
			// 	.huge_page_name = nullptr};
			// rpmalloc_initialize_config(&config);
			break;
		}
		default:
			assert(!"Unknown allocator type");
		}
	}
}

hot void *malloc(size_t Size)
{
	if (Size == 0)
	{
		error("Attempt of allocating 0 bytes in %s",
			  KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
								: "Unknown");
		assert(!"Attempt to allocate 0 bytes");
	}

	memdbg("malloc(%d)->[%s]", Size,
		   KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
							 : "Unknown");

	switch (AllocatorType)
	{
	case MemoryAllocatorType::liballoc11:
		return PREFIX(malloc)(Size);
	case MemoryAllocatorType::rpmalloc_:
		return rpmalloc(Size);
	default:
		assert(!"Unknown allocator type");
	}
}

hot void *calloc(size_t n, size_t Size)
{
	if (Size == 0)
	{
		error("Attempt of allocating 0 bytes in %s",
			  KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
								: "Unknown");
		assert(!"Attempt to allocate 0 bytes");
	}

	memdbg("calloc(%d, %d)->[%s]", n, Size,
		   KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
							 : "Unknown");

	switch (AllocatorType)
	{
	case MemoryAllocatorType::liballoc11:
		return PREFIX(calloc)(n, Size);
	case MemoryAllocatorType::rpmalloc_:
		return rpcalloc(n, Size);
	default:
		assert(!"Unknown allocator type");
	}
}

hot void *realloc(void *Address, size_t Size)
{
	if (Size == 0)
	{
		error("Attempt of allocating 0 bytes in %s",
			  KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
								: "Unknown");
		assert(!"Attempt to allocate 0 bytes");
	}

	memdbg("realloc(%#lx, %d)->[%s]", Address, Size,
		   KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
							 : "Unknown");

	switch (AllocatorType)
	{
	case MemoryAllocatorType::liballoc11:
		return PREFIX(realloc)(Address, Size);
	case MemoryAllocatorType::rpmalloc_:
		return rprealloc(Address, Size);
	default:
		assert(!"Unknown allocator type");
	}
}

hot void free(void *Address)
{
	if (Address == nullptr)
	{
		error("Attempt of freeing null pointer in %s",
			  KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
								: "Unknown");
		assert(!"Attempt to free a null pointer");
	}

	memdbg("free(%#lx)->[%s]", Address,
		   KernelSymbolTable ? KernelSymbolTable->GetSymbol((uintptr_t)__builtin_return_address(0))
							 : "Unknown");

	switch (AllocatorType)
	{
	case MemoryAllocatorType::liballoc11:
		PREFIX(free)(Address);
		break;
	case MemoryAllocatorType::rpmalloc_:
		rpfree(Address);
		break;
	default:
		assert(!"Unknown allocator type");
	}
}
