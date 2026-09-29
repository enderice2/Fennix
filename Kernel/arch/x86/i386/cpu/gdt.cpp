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
		.Null =
			{
				.Limit0 = 0x0,
				.BaseLow = 0x0,
				.BaseMiddle = 0x0,
				.Access = {.Raw = 0x0},
				// .Limit1 = 0x0,
				.Flags = {.Raw = 0x0},
				.BaseHigh = 0x0,
			},

		.Code =
			{
				.Limit0 = 0xFFFF,
				.BaseLow = 0x0,
				.BaseMiddle = 0x0,
				.Access = {
					.A = 0,
					.RW = 1,
					.DC = 0,
					.E = 1,
					.S = 1,
					.DPL = 0,
					.P = 1,
				},
				// .Limit1 = 0xF,
				.Flags = {
					.Reserved = 0xF, /* Workaround for Limit1 */

					.AVL = 0,
					.L = 0,
					.DB = 1,
					.G = 1,
				},
				.BaseHigh = 0x0,
			},

		.Data = {
			.Limit0 = 0xFFFF,
			.BaseLow = 0x0,
			.BaseMiddle = 0x0,
			.Access = {
				.A = 0,
				.RW = 1,
				.DC = 0,
				.E = 0,
				.S = 1,
				.DPL = 0,
				.P = 1,
			},
			// .Limit1 = 0xF,
			.Flags = {
				.Reserved = 0xF, /* Workaround for Limit1 */

				.AVL = 0,
				.L = 0,
				.DB = 1,
				.G = 1,
			},
			.BaseHigh = 0x0,
		},

		.UserData = {
			.Limit0 = 0xFFFF,
			.BaseLow = 0x0,
			.BaseMiddle = 0x0,
			.Access = {
				.A = 0,
				.RW = 1,
				.DC = 0,
				.E = 0,
				.S = 1,
				.DPL = 3,
				.P = 1,
			},
			// .Limit1 = 0xF,
			.Flags = {
				.Reserved = 0xF, /* Workaround for Limit1 */

				.AVL = 0,
				.L = 0,
				.DB = 1,
				.G = 1,
			},
			.BaseHigh = 0x0,
		},

		.UserCode = {
			.Limit0 = 0xFFFF,
			.BaseLow = 0x0,
			.BaseMiddle = 0x0,
			.Access = {
				.A = 0,
				.RW = 1,
				.DC = 0,
				.E = 1,
				.S = 1,
				.DPL = 3,
				.P = 1,
			},
			// .Limit1 = 0xF,
			.Flags = {
				.Reserved = 0xF, /* Workaround for Limit1 */

				.AVL = 0,
				.L = 0,
				.DB = 1,
				.G = 1,
			},
			.BaseHigh = 0x0,
		},

		.TaskStateSegment = {},
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

	nsa void Init(int Core)
	{
		memcpy(&GDTEntries[Core], &GDTEntriesTemplate, sizeof(GlobalDescriptorTableEntries));
		gdt[Core] = {.Length = sizeof(GlobalDescriptorTableEntries) - 1, .Entries = &GDTEntries[Core]};

		debug("GDT: %#lx", &gdt[Core]);
		debug("GDT KERNEL: CODE %#lx: Limit0: 0x%X, BaseLow: 0x%X, BaseMiddle: 0x%X, Access: 0x%X, Limit1: 0x%X, Flags: 0x%X, BaseHigh: 0x%X",
			  GDT_KERNEL_CODE,
			  GDTEntries[Core].Code.Limit0,
			  GDTEntries[Core].Code.BaseLow,
			  GDTEntries[Core].Code.BaseMiddle,
			  GDTEntries[Core].Code.Access.Raw,
			  GDTEntries[Core].Code.Flags.Reserved,
			  GDTEntries[Core].Code.Flags.Raw & ~0xF,
			  GDTEntries[Core].Code.BaseHigh);

		debug("GDT KERNEL: DATA %#lx: Limit0: 0x%X, BaseLow: 0x%X, BaseMiddle: 0x%X, Access: 0x%X, Limit1: 0x%X, Flags: 0x%X, BaseHigh: 0x%X",
			  GDT_KERNEL_DATA,
			  GDTEntries[Core].Data.Limit0,
			  GDTEntries[Core].Data.BaseLow,
			  GDTEntries[Core].Data.BaseMiddle,
			  GDTEntries[Core].Data.Access.Raw,
			  GDTEntries[Core].Data.Flags.Reserved,
			  GDTEntries[Core].Data.Flags.Raw & ~0xF,
			  GDTEntries[Core].Data.BaseHigh);

		debug("GDT USER: CODE %#lx: Limit0: 0x%X, BaseLow: 0x%X, BaseMiddle: 0x%X, Access: 0x%X, Limit1: 0x%X, Flags: 0x%X, BaseHigh: 0x%X",
			  GDT_USER_CODE,
			  GDTEntries[Core].UserCode.Limit0,
			  GDTEntries[Core].UserCode.BaseLow,
			  GDTEntries[Core].UserCode.BaseMiddle,
			  GDTEntries[Core].UserCode.Access.Raw,
			  GDTEntries[Core].UserCode.Flags.Reserved,
			  GDTEntries[Core].UserCode.Flags.Raw & ~0xF,
			  GDTEntries[Core].UserCode.BaseHigh);

		debug("GDT USER: DATA %#lx: Limit0: 0x%X, BaseLow: 0x%X, BaseMiddle: 0x%X, Access: 0x%X, Limit1: 0x%X, Flags: 0x%X, BaseHigh: 0x%X",
			  GDT_USER_DATA,
			  GDTEntries[Core].UserData.Limit0,
			  GDTEntries[Core].UserData.BaseLow,
			  GDTEntries[Core].UserData.BaseMiddle,
			  GDTEntries[Core].UserData.Access.Raw,
			  GDTEntries[Core].UserData.Flags.Reserved,
			  GDTEntries[Core].UserData.Flags.Raw & ~0xF,
			  GDTEntries[Core].UserData.BaseHigh);

		CPU::x32::lgdt(&gdt[Core]);

		asmv("mov %%esp, %%eax\n"
			 "push $16\n"
			 "push %%eax\n"
			 "pushf\n"
			 "push $8\n"
			 "push $1f\n"
			 "iret\n"
			 "1:\n"
			 "movw $16, %%ax\n"
			 "movw %%ax, %%ds\n"
			 "movw %%ax, %%es\n" :: : "memory",
			 "eax");

		CPUStackPointer[Core] = StackManager.Allocate(STACK_SIZE);
		memset(CPUStackPointer[Core], 0, STACK_SIZE);
		debug("CPU %d Stack Pointer: %#lx-%#lx (%d pages)", Core,
			  CPUStackPointer[Core], (uintptr_t)CPUStackPointer[Core] + STACK_SIZE,
			  TO_PAGES(STACK_SIZE + 1));

		uintptr_t Base = (uintptr_t)&tss[Core];
		size_t Limit = Base + sizeof(TaskStateSegment);
		gdt[Core].Entries->TaskStateSegment.Limit = Limit & 0xFFFF;
		gdt[Core].Entries->TaskStateSegment.BaseLow = Base & 0xFFFF;
		gdt[Core].Entries->TaskStateSegment.BaseMiddle = uint8_t((Base >> 16) & 0xFF);
		gdt[Core].Entries->TaskStateSegment.BaseHigh = uint8_t((Base >> 24) & 0xFF);

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
		gdt[Core].Entries->TaskStateSegment.BaseUpper = static_cast<uint32_t>((Base >> 32) & 0xFFFFFFFF);

		gdt[Core].Entries->TaskStateSegment.Access = {.A = 1, .RW = 0, .DC = 0, .E = 1, .S = 0, .DPL = 0, .P = 1};
		gdt[Core].Entries->TaskStateSegment.Granularity = (0 << 4) | ((Limit >> 16) & 0xF);

		tss[Core].IOMapBaseAddressOffset = sizeof(TaskStateSegment);
		tss[Core].StackPointer[0] = (uint32_t)CPUStackPointer[Core] + STACK_SIZE;
		tss[Core].StackPointer[1] = 0x0;
		tss[Core].StackPointer[2] = 0x0;

		for (size_t i = 0; i < sizeof(tss[Core].InterruptStackTable) / sizeof(tss[Core].InterruptStackTable[7]); i++)
		{
			void *NewStack = StackManager.Allocate(STACK_SIZE);

			tss[Core].InterruptStackTable[i] = (uint32_t)NewStack + STACK_SIZE;
			memset((void *)(tss[Core].InterruptStackTable[i] - STACK_SIZE), 0, STACK_SIZE);
			debug("IST-%d: %#lx-%#lx", i, NewStack, (uintptr_t)NewStack + STACK_SIZE);
		}

		CPU::x32::ltr(GDT_TSS);
		debug("Global Descriptor Table initialized");
	}

	nsa void SetKernelStack(void *Stack)
	{
		stub;
	}

	void *GetKernelStack() { return (void *)nullptr; }
}
