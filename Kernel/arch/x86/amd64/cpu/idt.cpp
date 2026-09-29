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

#include "idt.hpp"

#include <memory.hpp>
#include <cpu.hpp>
#include <debug.h>
#include <io.h>

#include "gdt.hpp"
#include "../../../../kernel.h"

extern "C" Ofast hot void interrupt_dispatch(CPU::TrapFrame *Frame) { irq.Dispatch(Frame); }
extern "C" Ofast hot void scheduler_dispatch(CPU::SchedulerFrame *Frame) { irq.Dispatch(Frame); }
extern "C" Ofast hot void exception_dispatch(CPU::ExceptionFrame *Frame) { irq.Dispatch(Frame); }

#define __stub_handler \
	__naked __used __no_stack_protector hot __aligned(16)

namespace InterruptDescriptorTable
{
	__aligned(8) static IDTGateDescriptor Entries[0x100];

	__aligned(8) IDTRegister IDTr = {
		.Limit = sizeof(Entries) - 1,
		.BaseAddress = Entries,
	};

	void SetEntry(uint8_t Index, void (*Base)(),
				  InterruptStackTableType InterruptStackTable,
				  GateType Gate, PrivilegeLevelType Ring,
				  bool Present, uint16_t SegmentSelector)
	{
		switch (Gate)
		{
		case CALL_GATE_64BIT:
		{
			CallGate gate{
				.TargetOffsetLow = static_cast<uint16_t>(((uint64_t)Base & 0xFFFF)),
				.TargetSelector = SegmentSelector,
				.Reserved0 = 0,
				.Type = Gate,
				.Zero0 = 0,
				.DescriptorPrivilegeLevel = Ring,
				.Present = Present,
				.TargetOffsetMiddle = static_cast<uint16_t>(((uint64_t)Base >> 16)),
				.TargetOffsetHigh = static_cast<uint32_t>(((uint64_t)Base >> 32)),
				.Reserved1 = 0,
				.Zero1 = 0,
				.Reserved2 = 0,
			};
			Entries[Index].Call = gate;
			break;
		}
		case INTERRUPT_GATE_64BIT:
		{
			InterruptGate gate{
				.TargetOffsetLow = static_cast<uint16_t>(((uint64_t)Base & 0xFFFF)),
				.TargetSelector = SegmentSelector,
				.InterruptStackTable = InterruptStackTable,
				.Reserved0 = 0,
				.Type = Gate,
				.Zero = 0,
				.DescriptorPrivilegeLevel = Ring,
				.Present = Present,
				.TargetOffsetMiddle = static_cast<uint16_t>(((uint64_t)Base >> 16)),
				.TargetOffsetHigh = static_cast<uint32_t>(((uint64_t)Base >> 32)),
				.Reserved1 = 0,
			};
			Entries[Index].Interrupt = gate;
			break;
		}
		case TRAP_GATE_64BIT:
		{
			TrapGate gate{
				.TargetOffsetLow = static_cast<uint16_t>(((uint64_t)Base & 0xFFFF)),
				.TargetSelector = SegmentSelector,
				.InterruptStackTable = InterruptStackTable,
				.Reserved0 = 0,
				.Type = Gate,
				.Zero = 0,
				.DescriptorPrivilegeLevel = Ring,
				.Present = Present,
				.TargetOffsetMiddle = static_cast<uint16_t>(((uint64_t)Base >> 16)),
				.TargetOffsetHigh = static_cast<uint32_t>(((uint64_t)Base >> 32)),
				.Reserved1 = 0,
			};
			Entries[Index].Trap = gate;
			break;
		}
		case LDT_64BIT:
		case AVAILABLE_64BIT_TSS:
		case BUSY_64BIT_TSS:
		default:
		{
			assert(!"Unsupported gate type");
			break;
		}
		}
	}

	extern "C" __stub_handler void ExceptionHandlerStub()
	{
		asm("cld\n"
			"cli\n"

			"pushq %rax\n"
			"pushq %rbx\n"
			"pushq %rcx\n"
			"pushq %rdx\n"
			"pushq %rsi\n"
			"pushq %rdi\n"
			"pushq %rbp\n"

			"pushq %r8\n"
			"pushq %r9\n"
			"pushq %r10\n"
			"pushq %r11\n"
			"pushq %r12\n"
			"pushq %r13\n"
			"pushq %r14\n"
			"pushq %r15\n"

			// "movq %ds, %rax\n pushq %rax\n"
			// "movq %es, %rax\n pushq %rax\n"
			// "movq %fs, %rax\n pushq %rax\n"
			// "movq %gs, %rax\n pushq %rax\n"
			"pushq %rax\n"
			"pushq %rax\n"
			"pushq %rax\n"
			"pushq %rax\n"

			"movq %dr7, %rax\n pushq %rax\n"
			"movq %dr6, %rax\n pushq %rax\n"
			"movq %dr3, %rax\n pushq %rax\n"
			"movq %dr2, %rax\n pushq %rax\n"
			"movq %dr1, %rax\n pushq %rax\n"
			"movq %dr0, %rax\n pushq %rax\n"

			"movq %cr8, %rax\n pushq %rax\n"
			"movq %cr4, %rax\n pushq %rax\n"
			"movq %cr3, %rax\n pushq %rax\n"
			"movq %cr2, %rax\n pushq %rax\n"
			"movq %cr0, %rax\n pushq %rax\n"

			"movq %rsp, %rdi\n"
			"call exception_dispatch\n"

			"popq %rax\n movq %rax, %cr0\n"
			"popq %rax\n movq %rax, %cr2\n"
			"popq %rax\n movq %rax, %cr3\n"
			"popq %rax\n movq %rax, %cr4\n"
			"popq %rax\n movq %rax, %cr8\n"

			"popq %rax\n movq %rax, %dr0\n"
			"popq %rax\n movq %rax, %dr1\n"
			"popq %rax\n movq %rax, %dr2\n"
			"popq %rax\n movq %rax, %dr3\n"
			"popq %rax\n movq %rax, %dr6\n"
			"popq %rax\n movq %rax, %dr7\n"

			"popq %rax\n"
			"popq %rax\n"
			"popq %rax\n"
			"popq %rax\n"
			// "popq %rax\n movq %rax, %gs\n"
			// "popq %rax\n movq %rax, %fs\n"
			// "popq %rax\n movq %rax, %es\n"
			// "popq %rax\n movq %rax, %ds\n"

			"popq %r15\n"
			"popq %r14\n"
			"popq %r13\n"
			"popq %r12\n"
			"popq %r11\n"
			"popq %r10\n"
			"popq %r9\n"
			"popq %r8\n"

			"popq %rbp\n"
			"popq %rdi\n"
			"popq %rsi\n"
			"popq %rdx\n"
			"popq %rcx\n"
			"popq %rbx\n"
			"popq %rax\n"

			"addq $16, %rsp\n"

			"iretq"); // pop CS RIP RFLAGS SS RSP
	}

	extern "C" __stub_handler void InterruptHandlerStub()
	{
		asm("cld\n"
			"cli\n"

			"pushq %rax\n"
			"pushq %rbx\n"
			"pushq %rcx\n"
			"pushq %rdx\n"
			"pushq %rsi\n"
			"pushq %rdi\n"
			"pushq %rbp\n"

			"pushq %r8\n"
			"pushq %r9\n"
			"pushq %r10\n"
			"pushq %r11\n"
			"pushq %r12\n"
			"pushq %r13\n"
			"pushq %r14\n"
			"pushq %r15\n"

			"movq %rsp, %rdi\n"
			"call interrupt_dispatch\n"

			"popq %r15\n"
			"popq %r14\n"
			"popq %r13\n"
			"popq %r12\n"
			"popq %r11\n"
			"popq %r10\n"
			"popq %r9\n"
			"popq %r8\n"

			"popq %rbp\n"
			"popq %rdi\n"
			"popq %rsi\n"
			"popq %rdx\n"
			"popq %rcx\n"
			"popq %rbx\n"
			"popq %rax\n"

			"addq $16, %rsp\n"

			"sti\n"
			"iretq"); // pop CS RIP RFLAGS SS RSP
	}

	extern "C" __stub_handler void SchedulerHandlerStub()
	{
		asm("cld\n"
			"cli\n"

			"pushq %rax\n"
			"pushq %rbx\n"
			"pushq %rcx\n"
			"pushq %rdx\n"
			"pushq %rsi\n"
			"pushq %rdi\n"
			"pushq %rbp\n"

			"pushq %r8\n"
			"pushq %r9\n"
			"pushq %r10\n"
			"pushq %r11\n"
			"pushq %r12\n"
			"pushq %r13\n"
			"pushq %r14\n"
			"pushq %r15\n"

			/* TODO: Add advanced check so we won't update the cr3 when not needed */

			"movq %cr3, %rax\n pushq %rax\n" /* Push opt */
			"pushq %rax\n"					 /* Push ppt */

			"movq %rsp, %rdi\n"
			"call scheduler_dispatch\n"

			"popq %rax\n movq %rax, %cr3\n" /* Restore to ppt */
			"popq %rax\n"					/* Pop opt */

			"popq %r15\n"
			"popq %r14\n"
			"popq %r13\n"
			"popq %r12\n"
			"popq %r11\n"
			"popq %r10\n"
			"popq %r9\n"
			"popq %r8\n"

			"popq %rbp\n"
			"popq %rdi\n"
			"popq %rsi\n"
			"popq %rdx\n"
			"popq %rcx\n"
			"popq %rbx\n"
			"popq %rax\n"

			"addq $16, %rsp\n"

			"sti\n"
			"iretq"); // pop CS RIP RFLAGS SS RSP
	}

#pragma region Interrupt Macros

#define EXCEPTION_HANDLER(num)                          \
	__stub_handler static void InterruptHandler_##num() \
	{                                                   \
		asm("pushq $0\n"                                \
			"pushq $" #num "\n"                         \
			"jmp ExceptionHandlerStub");                \
	}

#define EXCEPTION_ERROR_HANDLER(num)                    \
	__stub_handler static void InterruptHandler_##num() \
	{                                                   \
		asm("pushq $" #num "\n"                         \
			"jmp ExceptionHandlerStub");                \
	}

#define INTERRUPT_HANDLER(num)                   \
	__stub_handler void InterruptHandler_##num() \
	{                                            \
		asm("pushq $0\n"                         \
			"pushq $" #num "\n"                  \
			"jmp InterruptHandlerStub\n");       \
	}

#define SCHEDULER_HANDLER(num)                   \
	__stub_handler void InterruptHandler_##num() \
	{                                            \
		asm("pushq $0\n"                         \
			"pushq $" #num "\n"                  \
			"jmp SchedulerHandlerStub\n");       \
	}

	/* ISR */

	EXCEPTION_HANDLER(0x0);
	EXCEPTION_HANDLER(0x1);
	EXCEPTION_HANDLER(0x2);
	EXCEPTION_HANDLER(0x3);
	EXCEPTION_HANDLER(0x4);
	EXCEPTION_HANDLER(0x5);
	EXCEPTION_HANDLER(0x6);
	EXCEPTION_HANDLER(0x7);
	EXCEPTION_ERROR_HANDLER(0x8);
	EXCEPTION_HANDLER(0x9);
	EXCEPTION_ERROR_HANDLER(0xa);
	EXCEPTION_ERROR_HANDLER(0xb);
	EXCEPTION_ERROR_HANDLER(0xc);
	EXCEPTION_ERROR_HANDLER(0xd);
	EXCEPTION_ERROR_HANDLER(0xe);
	EXCEPTION_HANDLER(0xf);
	EXCEPTION_ERROR_HANDLER(0x10);
	EXCEPTION_HANDLER(0x11);
	EXCEPTION_HANDLER(0x12);
	EXCEPTION_HANDLER(0x13);
	EXCEPTION_HANDLER(0x14);
	EXCEPTION_HANDLER(0x15);
	EXCEPTION_HANDLER(0x16);
	EXCEPTION_HANDLER(0x17);
	EXCEPTION_HANDLER(0x18);
	EXCEPTION_HANDLER(0x19);
	EXCEPTION_HANDLER(0x1a);
	EXCEPTION_HANDLER(0x1b);
	EXCEPTION_HANDLER(0x1c);
	EXCEPTION_HANDLER(0x1d);
	EXCEPTION_HANDLER(0x1e);
	EXCEPTION_HANDLER(0x1f);

	/* IRQ */

	INTERRUPT_HANDLER(0x20)
	INTERRUPT_HANDLER(0x21)
	INTERRUPT_HANDLER(0x22)
	INTERRUPT_HANDLER(0x23)
	INTERRUPT_HANDLER(0x24)
	INTERRUPT_HANDLER(0x25)
	INTERRUPT_HANDLER(0x26)
	INTERRUPT_HANDLER(0x27)
	INTERRUPT_HANDLER(0x28)
	INTERRUPT_HANDLER(0x29)
	INTERRUPT_HANDLER(0x2a)
	INTERRUPT_HANDLER(0x2b)
	INTERRUPT_HANDLER(0x2c)
	INTERRUPT_HANDLER(0x2d)
	INTERRUPT_HANDLER(0x2e)
	INTERRUPT_HANDLER(0x2f)

	/* Reserved by OS */

	SCHEDULER_HANDLER(0x30)
	INTERRUPT_HANDLER(0x31)
	INTERRUPT_HANDLER(0x32)
	INTERRUPT_HANDLER(0x33)
	INTERRUPT_HANDLER(0x34)
	INTERRUPT_HANDLER(0x35)
	INTERRUPT_HANDLER(0x36)
	INTERRUPT_HANDLER(0x37)
	INTERRUPT_HANDLER(0x38)
	INTERRUPT_HANDLER(0x39)
	INTERRUPT_HANDLER(0x3a)
	INTERRUPT_HANDLER(0x3b)
	INTERRUPT_HANDLER(0x3c)
	INTERRUPT_HANDLER(0x3d)

	/* Free */

	INTERRUPT_HANDLER(0x3e)
	INTERRUPT_HANDLER(0x3f)
	INTERRUPT_HANDLER(0x40)
	INTERRUPT_HANDLER(0x41)
	INTERRUPT_HANDLER(0x42)
	INTERRUPT_HANDLER(0x43)
	INTERRUPT_HANDLER(0x44)
	INTERRUPT_HANDLER(0x45)
	INTERRUPT_HANDLER(0x46)
	INTERRUPT_HANDLER(0x47)
	INTERRUPT_HANDLER(0x48)
	INTERRUPT_HANDLER(0x49)
	INTERRUPT_HANDLER(0x4a)
	INTERRUPT_HANDLER(0x4b)
	INTERRUPT_HANDLER(0x4c)
	INTERRUPT_HANDLER(0x4d)
	INTERRUPT_HANDLER(0x4e)
	INTERRUPT_HANDLER(0x4f)
	INTERRUPT_HANDLER(0x50)
	INTERRUPT_HANDLER(0x51)
	INTERRUPT_HANDLER(0x52)
	INTERRUPT_HANDLER(0x53)
	INTERRUPT_HANDLER(0x54)
	INTERRUPT_HANDLER(0x55)
	INTERRUPT_HANDLER(0x56)
	INTERRUPT_HANDLER(0x57)
	INTERRUPT_HANDLER(0x58)
	INTERRUPT_HANDLER(0x59)
	INTERRUPT_HANDLER(0x5a)
	INTERRUPT_HANDLER(0x5b)
	INTERRUPT_HANDLER(0x5c)
	INTERRUPT_HANDLER(0x5d)
	INTERRUPT_HANDLER(0x5e)
	INTERRUPT_HANDLER(0x5f)
	INTERRUPT_HANDLER(0x60)
	INTERRUPT_HANDLER(0x61)
	INTERRUPT_HANDLER(0x62)
	INTERRUPT_HANDLER(0x63)
	INTERRUPT_HANDLER(0x64)
	INTERRUPT_HANDLER(0x65)
	INTERRUPT_HANDLER(0x66)
	INTERRUPT_HANDLER(0x67)
	INTERRUPT_HANDLER(0x68)
	INTERRUPT_HANDLER(0x69)
	INTERRUPT_HANDLER(0x6a)
	INTERRUPT_HANDLER(0x6b)
	INTERRUPT_HANDLER(0x6c)
	INTERRUPT_HANDLER(0x6d)
	INTERRUPT_HANDLER(0x6e)
	INTERRUPT_HANDLER(0x6f)
	INTERRUPT_HANDLER(0x70)
	INTERRUPT_HANDLER(0x71)
	INTERRUPT_HANDLER(0x72)
	INTERRUPT_HANDLER(0x73)
	INTERRUPT_HANDLER(0x74)
	INTERRUPT_HANDLER(0x75)
	INTERRUPT_HANDLER(0x76)
	INTERRUPT_HANDLER(0x77)
	INTERRUPT_HANDLER(0x78)
	INTERRUPT_HANDLER(0x79)
	INTERRUPT_HANDLER(0x7a)
	INTERRUPT_HANDLER(0x7b)
	INTERRUPT_HANDLER(0x7c)
	INTERRUPT_HANDLER(0x7d)
	INTERRUPT_HANDLER(0x7e)
	INTERRUPT_HANDLER(0x7f)
	INTERRUPT_HANDLER(0x80)
	INTERRUPT_HANDLER(0x81)
	INTERRUPT_HANDLER(0x82)
	INTERRUPT_HANDLER(0x83)
	INTERRUPT_HANDLER(0x84)
	INTERRUPT_HANDLER(0x85)
	INTERRUPT_HANDLER(0x86)
	INTERRUPT_HANDLER(0x87)
	INTERRUPT_HANDLER(0x88)
	INTERRUPT_HANDLER(0x89)
	INTERRUPT_HANDLER(0x8a)
	INTERRUPT_HANDLER(0x8b)
	INTERRUPT_HANDLER(0x8c)
	INTERRUPT_HANDLER(0x8d)
	INTERRUPT_HANDLER(0x8e)
	INTERRUPT_HANDLER(0x8f)
	INTERRUPT_HANDLER(0x90)
	INTERRUPT_HANDLER(0x91)
	INTERRUPT_HANDLER(0x92)
	INTERRUPT_HANDLER(0x93)
	INTERRUPT_HANDLER(0x94)
	INTERRUPT_HANDLER(0x95)
	INTERRUPT_HANDLER(0x96)
	INTERRUPT_HANDLER(0x97)
	INTERRUPT_HANDLER(0x98)
	INTERRUPT_HANDLER(0x99)
	INTERRUPT_HANDLER(0x9a)
	INTERRUPT_HANDLER(0x9b)
	INTERRUPT_HANDLER(0x9c)
	INTERRUPT_HANDLER(0x9d)
	INTERRUPT_HANDLER(0x9e)
	INTERRUPT_HANDLER(0x9f)
	INTERRUPT_HANDLER(0xa0)
	INTERRUPT_HANDLER(0xa1)
	INTERRUPT_HANDLER(0xa2)
	INTERRUPT_HANDLER(0xa3)
	INTERRUPT_HANDLER(0xa4)
	INTERRUPT_HANDLER(0xa5)
	INTERRUPT_HANDLER(0xa6)
	INTERRUPT_HANDLER(0xa7)
	INTERRUPT_HANDLER(0xa8)
	INTERRUPT_HANDLER(0xa9)
	INTERRUPT_HANDLER(0xaa)
	INTERRUPT_HANDLER(0xab)
	INTERRUPT_HANDLER(0xac)
	INTERRUPT_HANDLER(0xad)
	INTERRUPT_HANDLER(0xae)
	INTERRUPT_HANDLER(0xaf)
	INTERRUPT_HANDLER(0xb0)
	INTERRUPT_HANDLER(0xb1)
	INTERRUPT_HANDLER(0xb2)
	INTERRUPT_HANDLER(0xb3)
	INTERRUPT_HANDLER(0xb4)
	INTERRUPT_HANDLER(0xb5)
	INTERRUPT_HANDLER(0xb6)
	INTERRUPT_HANDLER(0xb7)
	INTERRUPT_HANDLER(0xb8)
	INTERRUPT_HANDLER(0xb9)
	INTERRUPT_HANDLER(0xba)
	INTERRUPT_HANDLER(0xbb)
	INTERRUPT_HANDLER(0xbc)
	INTERRUPT_HANDLER(0xbd)
	INTERRUPT_HANDLER(0xbe)
	INTERRUPT_HANDLER(0xbf)
	INTERRUPT_HANDLER(0xc0)
	INTERRUPT_HANDLER(0xc1)
	INTERRUPT_HANDLER(0xc2)
	INTERRUPT_HANDLER(0xc3)
	INTERRUPT_HANDLER(0xc4)
	INTERRUPT_HANDLER(0xc5)
	INTERRUPT_HANDLER(0xc6)
	INTERRUPT_HANDLER(0xc7)
	INTERRUPT_HANDLER(0xc8)
	INTERRUPT_HANDLER(0xc9)
	INTERRUPT_HANDLER(0xca)
	INTERRUPT_HANDLER(0xcb)
	INTERRUPT_HANDLER(0xcc)
	INTERRUPT_HANDLER(0xcd)
	INTERRUPT_HANDLER(0xce)
	INTERRUPT_HANDLER(0xcf)
	INTERRUPT_HANDLER(0xd0)
	INTERRUPT_HANDLER(0xd1)
	INTERRUPT_HANDLER(0xd2)
	INTERRUPT_HANDLER(0xd3)
	INTERRUPT_HANDLER(0xd4)
	INTERRUPT_HANDLER(0xd5)
	INTERRUPT_HANDLER(0xd6)
	INTERRUPT_HANDLER(0xd7)
	INTERRUPT_HANDLER(0xd8)
	INTERRUPT_HANDLER(0xd9)
	INTERRUPT_HANDLER(0xda)
	INTERRUPT_HANDLER(0xdb)
	INTERRUPT_HANDLER(0xdc)
	INTERRUPT_HANDLER(0xdd)
	INTERRUPT_HANDLER(0xde)
	INTERRUPT_HANDLER(0xdf)
	INTERRUPT_HANDLER(0xe0)
	INTERRUPT_HANDLER(0xe1)
	INTERRUPT_HANDLER(0xe2)
	INTERRUPT_HANDLER(0xe3)
	INTERRUPT_HANDLER(0xe4)
	INTERRUPT_HANDLER(0xe5)
	INTERRUPT_HANDLER(0xe6)
	INTERRUPT_HANDLER(0xe7)
	INTERRUPT_HANDLER(0xe8)
	INTERRUPT_HANDLER(0xe9)
	INTERRUPT_HANDLER(0xea)
	INTERRUPT_HANDLER(0xeb)
	INTERRUPT_HANDLER(0xec)
	INTERRUPT_HANDLER(0xed)
	INTERRUPT_HANDLER(0xee)
	INTERRUPT_HANDLER(0xef)
	INTERRUPT_HANDLER(0xf0)
	INTERRUPT_HANDLER(0xf1)
	INTERRUPT_HANDLER(0xf2)
	INTERRUPT_HANDLER(0xf3)
	INTERRUPT_HANDLER(0xf4)
	INTERRUPT_HANDLER(0xf5)
	INTERRUPT_HANDLER(0xf6)
	INTERRUPT_HANDLER(0xf7)
	INTERRUPT_HANDLER(0xf8)
	INTERRUPT_HANDLER(0xf9)
	INTERRUPT_HANDLER(0xfa)
	INTERRUPT_HANDLER(0xfb)
	INTERRUPT_HANDLER(0xfc)
	INTERRUPT_HANDLER(0xfd)
	INTERRUPT_HANDLER(0xfe)
	INTERRUPT_HANDLER(0xff)

#pragma endregion Interrupt Macros

	void Setup(int Core)
	{
		if (Core != 0)
		{
			CPU::x64::lidt(&IDTr);
			return;
		}

		bool EnableISRs = true;
#ifdef DEBUG
		EnableISRs = !DebuggerIsAttached;
		if (!EnableISRs)
			klog("\x1b[32mThe debugger is attached, disabling all ISRs.");
#endif

		/* ISR */
		SetEntry(0x0, InterruptHandler_0x0, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1, InterruptHandler_0x1, IST1, INTERRUPT_GATE_64BIT, RING3, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x2, InterruptHandler_0x2, IST2, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x3, InterruptHandler_0x3, IST1, INTERRUPT_GATE_64BIT, RING3, (!DebuggerIsAttached), GDT_KERNEL_CODE); /* Do not handle breakpoints if we are debugging the kernel. */
		SetEntry(0x4, InterruptHandler_0x4, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x5, InterruptHandler_0x5, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x6, InterruptHandler_0x6, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x7, InterruptHandler_0x7, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x8, InterruptHandler_0x8, IST3, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x9, InterruptHandler_0x9, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0xa, InterruptHandler_0xa, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0xb, InterruptHandler_0xb, IST1, INTERRUPT_GATE_64BIT, RING0, (!DebuggerIsAttached), GDT_KERNEL_CODE);
		SetEntry(0xc, InterruptHandler_0xc, IST3, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0xd, InterruptHandler_0xd, IST3, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0xe, InterruptHandler_0xe, IST3, INTERRUPT_GATE_64BIT, RING0, true /* FIXME: CoW? */, GDT_KERNEL_CODE);
		SetEntry(0xf, InterruptHandler_0xf, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x10, InterruptHandler_0x10, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x11, InterruptHandler_0x11, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x12, InterruptHandler_0x12, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x13, InterruptHandler_0x13, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x14, InterruptHandler_0x14, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x15, InterruptHandler_0x15, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x16, InterruptHandler_0x16, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x17, InterruptHandler_0x17, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x18, InterruptHandler_0x18, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x19, InterruptHandler_0x19, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1a, InterruptHandler_0x1a, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1b, InterruptHandler_0x1b, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1c, InterruptHandler_0x1c, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1d, InterruptHandler_0x1d, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1e, InterruptHandler_0x1e, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);
		SetEntry(0x1f, InterruptHandler_0x1f, IST1, INTERRUPT_GATE_64BIT, RING0, EnableISRs, GDT_KERNEL_CODE);

		/* IRQ */

		SetEntry(0x20, InterruptHandler_0x20, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x21, InterruptHandler_0x21, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x22, InterruptHandler_0x22, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x23, InterruptHandler_0x23, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x24, InterruptHandler_0x24, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x25, InterruptHandler_0x25, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x26, InterruptHandler_0x26, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x27, InterruptHandler_0x27, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x28, InterruptHandler_0x28, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x29, InterruptHandler_0x29, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2a, InterruptHandler_0x2a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2b, InterruptHandler_0x2b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2c, InterruptHandler_0x2c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2d, InterruptHandler_0x2d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2e, InterruptHandler_0x2e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x2f, InterruptHandler_0x2f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);

		/* Reserved by OS */

		SetEntry(0x30, InterruptHandler_0x30, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x31, InterruptHandler_0x31, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x32, InterruptHandler_0x32, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x33, InterruptHandler_0x33, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x34, InterruptHandler_0x34, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x35, InterruptHandler_0x35, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x36, InterruptHandler_0x36, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x37, InterruptHandler_0x37, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x38, InterruptHandler_0x38, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x39, InterruptHandler_0x39, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x3a, InterruptHandler_0x3a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x3b, InterruptHandler_0x3b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x3c, InterruptHandler_0x3c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x3d, InterruptHandler_0x3d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);

		/* Free */

		SetEntry(0x3e, InterruptHandler_0x3e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x3f, InterruptHandler_0x3f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x40, InterruptHandler_0x40, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x41, InterruptHandler_0x41, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x42, InterruptHandler_0x42, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x43, InterruptHandler_0x43, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x44, InterruptHandler_0x44, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x45, InterruptHandler_0x45, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x46, InterruptHandler_0x46, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x47, InterruptHandler_0x47, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x48, InterruptHandler_0x48, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x49, InterruptHandler_0x49, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4a, InterruptHandler_0x4a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4b, InterruptHandler_0x4b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4c, InterruptHandler_0x4c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4d, InterruptHandler_0x4d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4e, InterruptHandler_0x4e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x4f, InterruptHandler_0x4f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x50, InterruptHandler_0x50, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x51, InterruptHandler_0x51, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x52, InterruptHandler_0x52, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x53, InterruptHandler_0x53, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x54, InterruptHandler_0x54, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x55, InterruptHandler_0x55, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x56, InterruptHandler_0x56, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x57, InterruptHandler_0x57, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x58, InterruptHandler_0x58, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x59, InterruptHandler_0x59, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5a, InterruptHandler_0x5a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5b, InterruptHandler_0x5b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5c, InterruptHandler_0x5c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5d, InterruptHandler_0x5d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5e, InterruptHandler_0x5e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x5f, InterruptHandler_0x5f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x60, InterruptHandler_0x60, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x61, InterruptHandler_0x61, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x62, InterruptHandler_0x62, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x63, InterruptHandler_0x63, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x64, InterruptHandler_0x64, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x65, InterruptHandler_0x65, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x66, InterruptHandler_0x66, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x67, InterruptHandler_0x67, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x68, InterruptHandler_0x68, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x69, InterruptHandler_0x69, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6a, InterruptHandler_0x6a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6b, InterruptHandler_0x6b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6c, InterruptHandler_0x6c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6d, InterruptHandler_0x6d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6e, InterruptHandler_0x6e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x6f, InterruptHandler_0x6f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x70, InterruptHandler_0x70, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x71, InterruptHandler_0x71, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x72, InterruptHandler_0x72, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x73, InterruptHandler_0x73, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x74, InterruptHandler_0x74, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x75, InterruptHandler_0x75, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x76, InterruptHandler_0x76, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x77, InterruptHandler_0x77, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x78, InterruptHandler_0x78, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x79, InterruptHandler_0x79, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7a, InterruptHandler_0x7a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7b, InterruptHandler_0x7b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7c, InterruptHandler_0x7c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7d, InterruptHandler_0x7d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7e, InterruptHandler_0x7e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x7f, InterruptHandler_0x7f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x80, InterruptHandler_0x80, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x81, InterruptHandler_0x81, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x82, InterruptHandler_0x82, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x83, InterruptHandler_0x83, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x84, InterruptHandler_0x84, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x85, InterruptHandler_0x85, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x86, InterruptHandler_0x86, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x87, InterruptHandler_0x87, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x88, InterruptHandler_0x88, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x89, InterruptHandler_0x89, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8a, InterruptHandler_0x8a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8b, InterruptHandler_0x8b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8c, InterruptHandler_0x8c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8d, InterruptHandler_0x8d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8e, InterruptHandler_0x8e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x8f, InterruptHandler_0x8f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x90, InterruptHandler_0x90, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x91, InterruptHandler_0x91, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x92, InterruptHandler_0x92, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x93, InterruptHandler_0x93, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x94, InterruptHandler_0x94, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x95, InterruptHandler_0x95, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x96, InterruptHandler_0x96, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x97, InterruptHandler_0x97, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x98, InterruptHandler_0x98, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x99, InterruptHandler_0x99, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9a, InterruptHandler_0x9a, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9b, InterruptHandler_0x9b, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9c, InterruptHandler_0x9c, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9d, InterruptHandler_0x9d, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9e, InterruptHandler_0x9e, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0x9f, InterruptHandler_0x9f, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa0, InterruptHandler_0xa0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa1, InterruptHandler_0xa1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa2, InterruptHandler_0xa2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa3, InterruptHandler_0xa3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa4, InterruptHandler_0xa4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa5, InterruptHandler_0xa5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa6, InterruptHandler_0xa6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa7, InterruptHandler_0xa7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa8, InterruptHandler_0xa8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xa9, InterruptHandler_0xa9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xaa, InterruptHandler_0xaa, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xab, InterruptHandler_0xab, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xac, InterruptHandler_0xac, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xad, InterruptHandler_0xad, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xae, InterruptHandler_0xae, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xaf, InterruptHandler_0xaf, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb0, InterruptHandler_0xb0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb1, InterruptHandler_0xb1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb2, InterruptHandler_0xb2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb3, InterruptHandler_0xb3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb4, InterruptHandler_0xb4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb5, InterruptHandler_0xb5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb6, InterruptHandler_0xb6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb7, InterruptHandler_0xb7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb8, InterruptHandler_0xb8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xb9, InterruptHandler_0xb9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xba, InterruptHandler_0xba, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xbb, InterruptHandler_0xbb, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xbc, InterruptHandler_0xbc, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xbd, InterruptHandler_0xbd, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xbe, InterruptHandler_0xbe, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xbf, InterruptHandler_0xbf, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc0, InterruptHandler_0xc0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc1, InterruptHandler_0xc1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc2, InterruptHandler_0xc2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc3, InterruptHandler_0xc3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc4, InterruptHandler_0xc4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc5, InterruptHandler_0xc5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc6, InterruptHandler_0xc6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc7, InterruptHandler_0xc7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc8, InterruptHandler_0xc8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xc9, InterruptHandler_0xc9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xca, InterruptHandler_0xca, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xcb, InterruptHandler_0xcb, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xcc, InterruptHandler_0xcc, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xcd, InterruptHandler_0xcd, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xce, InterruptHandler_0xce, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xcf, InterruptHandler_0xcf, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd0, InterruptHandler_0xd0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd1, InterruptHandler_0xd1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd2, InterruptHandler_0xd2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd3, InterruptHandler_0xd3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd4, InterruptHandler_0xd4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd5, InterruptHandler_0xd5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd6, InterruptHandler_0xd6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd7, InterruptHandler_0xd7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd8, InterruptHandler_0xd8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xd9, InterruptHandler_0xd9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xda, InterruptHandler_0xda, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xdb, InterruptHandler_0xdb, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xdc, InterruptHandler_0xdc, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xdd, InterruptHandler_0xdd, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xde, InterruptHandler_0xde, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xdf, InterruptHandler_0xdf, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe0, InterruptHandler_0xe0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe1, InterruptHandler_0xe1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe2, InterruptHandler_0xe2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe3, InterruptHandler_0xe3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe4, InterruptHandler_0xe4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe5, InterruptHandler_0xe5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe6, InterruptHandler_0xe6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe7, InterruptHandler_0xe7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe8, InterruptHandler_0xe8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xe9, InterruptHandler_0xe9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xea, InterruptHandler_0xea, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xeb, InterruptHandler_0xeb, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xec, InterruptHandler_0xec, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xed, InterruptHandler_0xed, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xee, InterruptHandler_0xee, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xef, InterruptHandler_0xef, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf0, InterruptHandler_0xf0, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf1, InterruptHandler_0xf1, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf2, InterruptHandler_0xf2, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf3, InterruptHandler_0xf3, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf4, InterruptHandler_0xf4, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf5, InterruptHandler_0xf5, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf6, InterruptHandler_0xf6, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf7, InterruptHandler_0xf7, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf8, InterruptHandler_0xf8, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xf9, InterruptHandler_0xf9, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xfa, InterruptHandler_0xfa, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xfb, InterruptHandler_0xfb, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xfc, InterruptHandler_0xfc, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xfd, InterruptHandler_0xfd, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xfe, InterruptHandler_0xfe, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		SetEntry(0xff, InterruptHandler_0xff, IST0, INTERRUPT_GATE_64BIT, RING0, true, GDT_KERNEL_CODE);
		CPU::x64::lidt(&IDTr);
	}
}
