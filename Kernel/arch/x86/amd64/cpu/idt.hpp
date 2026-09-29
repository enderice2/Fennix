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

#ifndef __FENNIX_KERNEL_IDT_H__
#define __FENNIX_KERNEL_IDT_H__

#include <types.h>
#include <cpu/x86/x64/SegmentDescriptors.hpp>

namespace InterruptDescriptorTable
{
	union IDTGateDescriptor
	{
		InterruptGate Interrupt;
		TrapGate Trap;
		CallGate Call;
	};

	struct IDTRegister
	{
		uint16_t Limit;
		IDTGateDescriptor *BaseAddress;
	} __packed;

	void SetEntry(uint8_t Index,
				  void (*Base)(),
				  InterruptStackTableType InterruptStackTable,
				  GateType Gate,
				  PrivilegeLevelType Ring,
				  bool Present,
				  uint16_t SegmentSelector);

	void Setup(int Core);
}

#endif // !__FENNIX_KERNEL_IDT_H__
