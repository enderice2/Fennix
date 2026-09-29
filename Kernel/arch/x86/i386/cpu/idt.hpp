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

namespace InterruptDescriptorTable
{
	/**
	 * Manual: AMD Architecture Programmer's Manual Volume 2: System Programming
	 * Subsection: 4.7.4 System Descriptors
	 * Table: 4-5
	 *
	 * @note Reserved values are not listed in the table.
	 */
	enum GateType
	{
		AVAILABLE_16BIT_TSS = 0b0001,
		LDT = 0b0010,
		BUSY_16BIT_TSS = 0b0011,
		CALL_GATE_16BIT = 0b0100,
		TASK_GATE = 0b0101,
		INTERRUPT_GATE_16BIT = 0b0110,
		TRAP_GATE_16BIT = 0b0111,
		AVAILABLE_32BIT_TSS = 0b1001,
		BUSY_32BIT_TSS = 0b1011,
		CALL_GATE_32BIT = 0b1100,
		INTERRUPT_GATE_32BIT = 0b1110,
		TRAP_GATE_32BIT = 0b1111,
	};

	enum PrivilegeLevelType
	{
		RING0 = 0b0,
		RING1 = 0b1,
		RING2 = 0b10,
		RING3 = 0b11,
	};

	struct LDTDescriptor
	{
		/* +0 */
		uint32_t SegmentLimitLow : 16;
		uint32_t BaseAddressLow : 16;
		/* +4 */
		uint32_t BaseAddressMiddle : 8;
		uint32_t Type : 4;
		uint32_t Zero : 1;
		uint32_t DescriptorPrivilegeLevel : 2;
		uint32_t Present : 1;
		uint32_t SegmentLimitHigh : 4;
		uint32_t Available : 1;
		uint32_t Zero1 : 2;
		uint32_t Granularity : 1;
		uint32_t BaseAddressHigh : 8;
	} __packed;

	typedef LDTDescriptor TSSDescriptor;

	struct CallGate
	{
		/* +0 */
		uint32_t TargetCodeSegmentOffsetLow : 16;
		uint32_t TargetCodeSegmentSelector : 16;
		/* +4 */
		uint32_t ParameterCount : 4;
		uint32_t Reserved0 : 3;
		uint32_t Type : 4;
		uint32_t Zero : 1;
		uint32_t DescriptorPrivilegeLevel : 2;
		uint32_t Present : 1;
		uint32_t TargetCodeSegmentOffsetHigh : 16;
	} __packed;

	struct InterruptGate
	{
		/* +0 */
		uint32_t TargetCodeSegmentOffsetLow : 16;
		uint32_t TargetCodeSegmentSelector : 16;
		/* +4 */
		uint32_t Reserved0 : 8;
		uint32_t Type : 4;
		uint32_t Zero : 1;
		uint32_t DescriptorPrivilegeLevel : 2;
		uint32_t Present : 1;
		uint32_t TargetCodeSegmentOffsetHigh : 16;
	} __packed;

	typedef InterruptGate TrapGate;

	struct TaskGate
	{
		/* +0 */
		uint32_t Reserved0 : 16;
		uint32_t TSSSelector : 16;
		/* +4 */
		uint32_t Reserved1 : 8;
		uint32_t Type : 4;
		uint32_t Zero : 1;
		uint32_t DescriptorPrivilegeLevel : 2;
		uint32_t Present : 1;
		uint32_t Reserved2 : 16;
	} __packed;

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
				  GateType Gate,
				  PrivilegeLevelType Ring,
				  bool Present,
				  uint16_t SegmentSelector);

	void Init(int Core);
}

#endif // !__FENNIX_KERNEL_IDT_H__
