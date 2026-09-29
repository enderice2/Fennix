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

#include "gdt.hpp"

#include <memory.hpp>
#include <smp.hpp>
#include <cpu.hpp>
#include <debug.h>

namespace GlobalDescriptorTable
{
	static GlobalDescriptorTableEntries GDTEntriesTemplate = {
		.Null = 0,
		.Code = {
			.SegmentLimitLow = 0xFFFF,
			.BaseAddressLow = 0x0,
			.BaseAddressHigh = 0x0,
			.Accessed = 0,
			.Readable = 1,
			.Conforming = 0,
			.Executable = 1,
			.Type = 1,
			.DescriptorPrivilegeLevel = 0,
			.Present = 1,
			.SegmentLimitHigh = 0xF,
			.Available = 0,
			.Long = 1,
			.Default = 0,
			.Granularity = 1,
			.BaseAddressHigher = 0x0,
		},

		.Data = {
			.SegmentLimitLow = 0xFFFF,
			.BaseAddressLow = 0x0,
			.BaseAddressHigh = 0x0,
			.Accessed = 0,
			.Writable = 1,
			.ExpandDown = 0,
			.Executable = 0,
			.Type = 1,
			.DescriptorPrivilegeLevel = 0,
			.Present = 1,
			.SegmentLimitHigh = 0xF,
			.Available = 0,
			.Reserved = 0,
			.Default = 0,
			.Granularity = 1,
			.BaseAddressHigher = 0x0,
		},

		.UserData = {
			.SegmentLimitLow = 0xFFFF,
			.BaseAddressLow = 0x0,
			.BaseAddressHigh = 0x0,
			.Accessed = 0,
			.Writable = 1,
			.ExpandDown = 1,
			.Executable = 0,
			.Type = 1,
			.DescriptorPrivilegeLevel = 3,
			.Present = 1,
			.SegmentLimitHigh = 0xF,
			.Available = 0,
			.Reserved = 0,
			.Default = 0,
			.Granularity = 1,
			.BaseAddressHigher = 0x0,
		},

		.UserCode = {
			.SegmentLimitLow = 0xFFFF,
			.BaseAddressLow = 0x0,
			.BaseAddressHigh = 0x0,
			.Accessed = 0,
			.Readable = 1,
			.Conforming = 0,
			.Executable = 1,
			.Type = 1,
			.DescriptorPrivilegeLevel = 3,
			.Present = 1,
			.SegmentLimitHigh = 0xF,
			.Available = 0,
			.Long = 1,
			.Default = 0,
			.Granularity = 1,
			.BaseAddressHigher = 0x0,
		},

		.TaskStateSegment{},
	};

	GlobalDescriptorTableEntries GDTEntries[MAX_CPU] __aligned(16);
	GlobalDescriptorTableDescriptor gdt[MAX_CPU] __aligned(16);

	TaskStateSegment tss[MAX_CPU] = {
		0,
		{0, 0, 0},
		0,
		{0, 0, 0, 0, 0, 0, 0},
		0,
		0,
		0,
	};

	void *CPUStackPointer[MAX_CPU];

	nsa void Setup(int Core)
	{
		GDTEntries[Core] = GDTEntriesTemplate;
		gdt[Core] =
			{
				.Limit = sizeof(GlobalDescriptorTableEntries) - 1,
				.BaseAddress = &GDTEntries[Core],
			};

		debug("GDT: %#lx", &gdt[Core]);
		debug("GDT KERNEL CODE %#lx", GDT_KERNEL_CODE);
		debug("GDT KERNEL DATA %#lx", GDT_KERNEL_DATA);
		debug("GDT  USER  CODE %#lx", GDT_USER_CODE);
		debug("GDT  USER  DATA %#lx", GDT_USER_DATA);
		debug("GDT     TSS     %#lx", GDT_TSS);

		CPU::x64::lgdt(&gdt[Core]);

		asmv("movq %%rsp, %%rax\n"
			 "pushq $16\n"
			 "pushq %%rax\n"
			 "pushfq\n"
			 "pushq $8\n"
			 "pushq $1f\n"
			 "iretq\n"
			 "1:\n"
			 "movw $16, %%ax\n"
			 "movw %%ax, %%ds\n"
			 "movw %%ax, %%es\n" :: : "memory",
			 "rax");

		CPUStackPointer[Core] = StackManager.Allocate(STACK_SIZE);
		memset(CPUStackPointer[Core], 0, STACK_SIZE);
		debug("CPU %d Stack Pointer: %#lx-%#lx (%d pages)", Core,
			  CPUStackPointer[Core], (uintptr_t)CPUStackPointer[Core] + STACK_SIZE,
			  TO_PAGES(STACK_SIZE + 1));

		uintptr_t Base = (uintptr_t)&tss[Core];
		size_t Limit = Base + sizeof(TaskStateSegment);
		SystemSegmentDescriptor *tssDesc = &gdt[Core].BaseAddress->TaskStateSegment;
		tssDesc->SegmentLimitLow = Limit & 0xFFFF;
		tssDesc->BaseAddressLow = Base & 0xFFFF;
		tssDesc->BaseAddressMiddle = (Base >> 16) & 0xFF;
		tssDesc->Type = AVAILABLE_64BIT_TSS;
		tssDesc->Zero0 = 0;
		tssDesc->DescriptorPrivilegeLevel = 0;
		tssDesc->Present = 1;
		tssDesc->Available = 0;
		tssDesc->Reserved0 = 0;
		tssDesc->Granularity = 0;
		tssDesc->BaseAddressHigh = (Base >> 24) & 0xFF;
		tssDesc->BaseAddressHigher = static_cast<uint32_t>((Base >> 32) & 0xFFFFFFFF);
		tssDesc->Reserved1 = 0;
		tssDesc->Zero1 = 0;
		tssDesc->Reserved2 = 0;

		tss[Core].IOMapBaseAddressOffset = sizeof(TaskStateSegment);
		tss[Core].StackPointer[0] = (uint64_t)CPUStackPointer[Core] + STACK_SIZE;
		tss[Core].StackPointer[1] = 0x0;
		tss[Core].StackPointer[2] = 0x0;

		for (size_t i = 0; i < numof(tss[Core].InterruptStackTable); i++)
		{
			void *newStack = StackManager.Allocate(STACK_SIZE);
			tss[Core].InterruptStackTable[i] = (uint64_t)newStack + STACK_SIZE;
			memset((void *)(tss[Core].InterruptStackTable[i] - STACK_SIZE), 0, STACK_SIZE);
			debug("IST-%d: %#lx-%#lx", i, newStack, (uintptr_t)newStack + STACK_SIZE);
		}

		CPU::x64::ltr(GDT_TSS);
		debug("Global Descriptor Table initialized");
	}

	void SetKernelStack(void *Stack)
	{
		long id = GetCurrentCPU()->ID;
		if (Stack != nullptr)
			tss[id].StackPointer[0] = (uint64_t)Stack;
		else
			tss[id].StackPointer[0] = (uint64_t)CPUStackPointer[id] + STACK_SIZE;

		/*
		FIXME: There's a bug in kernel which if
		we won't update "tss[id].StackPointer[0]"
		with the current stack pointer, the kernel
		will crash.

		2026 update: i still don't know
		*/
		asmv("mov %%rsp, %0" : "=r"(tss[id].StackPointer[0]));
	}

	void *GetKernelStack() { return (void *)tss[GetCurrentCPU()->ID].StackPointer[0]; }
}
