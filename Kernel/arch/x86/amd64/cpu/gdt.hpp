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

#ifndef __FENNIX_KERNEL_GDT_H__
#define __FENNIX_KERNEL_GDT_H__

#include <types.h>
#include <cpu/x86/x64/SegmentDescriptors.hpp>

namespace GlobalDescriptorTable
{
	struct TaskStateSegment
	{
		uint32_t Reserved0 __aligned(16);
		uint64_t StackPointer[3];
		uint64_t Reserved1;
		uint64_t InterruptStackTable[7];
		uint64_t Reserved2;
		uint16_t Reserved3;
		uint16_t IOMapBaseAddressOffset;
	} __packed;

	struct GlobalDescriptorTableEntries
	{
		uint64_t Null;
		CodeSegmentDescriptor Code;
		DataSegmentDescriptor Data;
		DataSegmentDescriptor UserData;
		CodeSegmentDescriptor UserCode;
		SystemSegmentDescriptor TaskStateSegment;
	} __packed;

	struct GlobalDescriptorTableDescriptor
	{
		uint16_t Limit;
		GlobalDescriptorTableEntries *BaseAddress;
	} __packed;

	extern void *CPUStackPointer[];
	extern TaskStateSegment tss[];

	void Setup(int Core);
	void SetKernelStack(void *Stack);
	void *GetKernelStack();
}

#define GDT_KERNEL_CODE offsetof(GlobalDescriptorTable::GlobalDescriptorTableEntries, Code)
#define GDT_KERNEL_DATA offsetof(GlobalDescriptorTable::GlobalDescriptorTableEntries, Data)
#define GDT_USER_CODE (offsetof(GlobalDescriptorTable::GlobalDescriptorTableEntries, UserCode) | 3)
#define GDT_USER_DATA (offsetof(GlobalDescriptorTable::GlobalDescriptorTableEntries, UserData) | 3)
#define GDT_TSS (offsetof(GlobalDescriptorTable::GlobalDescriptorTableEntries, TaskStateSegment) | 3)

#endif // !__FENNIX_KERNEL_GDT_H__
