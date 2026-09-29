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

#include <display.hpp>
#include <bitmap.hpp>
#include <convert.h>
#include <printf.h>
#include <lock.hpp>
#include <rand.hpp>
#include <uart.hpp>
#include <debug.h>
#include <smp.hpp>
#include <cpu.hpp>
#include <io.h>

#include "../../cpu/gdt.hpp"
#include "../../cpu/apic.hpp"
#include "../../../../kernel.h"

extern void ExPrint(const char *Format, ...);

static const char *x86PageFaultDescriptions[9] = {
	"Supervisor tried to read a non-present page entry\n",
	"Supervisor tried to read a page and caused a protection fault\n",
	"Supervisor tried to write to a non-present page entry\n",
	"Supervisor tried to write a page and caused a protection fault\n",
	"User tried to read a non-present page entry\n",
	"User tried to read a page and caused a protection fault\n",
	"User tried to write to a non-present page entry\n",
	"User tried to write a page and caused a protection fault\n",
	"One or more page directory entries contain reserved bits which are set to 1.\n"};

arch nsa void DisplayDetailsScreen(CPU::ExceptionFrame *Frame)
{
	ExPrint("\nException Frame:\n");
	ExPrint(" General Purpose Registers:\n");
	ExPrint("  EAX: %#lx  EBX: %#lx  ECX: %#lx  EDX: %#lx\n",
			Frame->eax, Frame->ebx, Frame->ecx, Frame->edx);
	ExPrint("  ESI: %#lx  EDI: %#lx\n",
			Frame->esi, Frame->edi);

	ExPrint(" Control Registers:\n");
	ExPrint("  CR0: %#lx  CR2: %#lx  CR3: %#lx  CR4: %#lx\n",
			Frame->cr0, Frame->cr2, Frame->cr3, Frame->cr4);
	ExPrint("  CR8: %#lx\n", Frame->cr8);

	ExPrint(" Segment Registers:\n");
	ExPrint("  CS: %#lx  SS: %#lx  DS: %#lx  ES: %#lx  FS: %#lx  GS: %#lx\n",
			Frame->cs, Frame->ss, Frame->ds, Frame->es, Frame->fs, Frame->gs);

	ExPrint(" Debug Registers:\n");
	ExPrint("  DR0: %#lx  DR1: %#lx  DR2: %#lx  DR3: %#lx\n",
			Frame->dr0, Frame->dr1, Frame->dr2, Frame->dr3);
	ExPrint("  DR6: %#lx  DR7: %#lx\n", Frame->dr6, Frame->dr7);

	ExPrint(" Other:\n");
	ExPrint("  INT: %#lx  ERR: %#lx  EIP: %#lx  RFLAGS: %#lx\n",
			Frame->InterruptNumber, Frame->ErrorCode,
			Frame->eip, Frame->eflags.raw);
	ExPrint("  RSP: %#lx  RBP: %#lx\n",
			Frame->esp, Frame->ebp);

	ExPrint("Exception Details:\n");
	switch (Frame->InterruptNumber)
	{
	case CPU::x86::PageFault:
	{
		CPU::x64::PageFaultErrorCode pfCode = {.raw = (uint32_t)Frame->ErrorCode};
		ExPrint("PFEC: P:%d W:%d U:%d R:%d I:%d PK:%d SS:%d SGX:%d\n",
				pfCode.P, pfCode.W, pfCode.U, pfCode.R,
				pfCode.I, pfCode.PK, pfCode.SS, pfCode.SGX);

		{
			Memory::Virtual vmm((Memory::PageTable *)Frame->cr3);
			if (vmm.GetMapType((void *)Frame->cr2) != Memory::Virtual::FourKiB)
				ExPrint("Can't display page %#lx\n", Frame->cr2);
			else
			{
				Memory::PageTableEntry *pte = vmm.GetPTE((void *)Frame->cr2);
				ExPrint("Page %#lx: P:%d W:%d U:%d G:%d CoW:%d KRsv:%d\n",
						ALIGN_DOWN(Frame->cr2, 0x1000), pte->Present, pte->ReadWrite,
						pte->UserSupervisor, pte->Global, pte->CopyOnWrite,
						pte->KernelReserve);
			}
		}

		ExPrint("%s", x86PageFaultDescriptions[Frame->ErrorCode & 0b111]);
		if (Frame->ErrorCode & 0x8)
			ExPrint("%s", x86PageFaultDescriptions[8]);
		break;
	}
	case CPU::x86::StackSegmentFault:
	case CPU::x86::GeneralProtectionFault:
	{
		CPU::x64::SelectorErrorCode sCode = {.raw = Frame->ErrorCode};
		ExPrint("Kernel performed an illegal operation.\n");
		ExPrint("External: %d\n", sCode.External);
		ExPrint("Table: %d\n", sCode.Table);
		ExPrint("Index: %#x\n", sCode.Idx);
		break;
	}
	default:
		ExPrint("No additional information available for this exception.\n");
		break;
	}

	if (!TaskManager)
		return;

	CPUData *core = GetCurrentCPU();
	Tasking::PCB *proc = core->CurrentProcess.load();
	Tasking::TCB *thread = core->CurrentThread.load();
	ExPrint("Exception in %s process %s(%d) thread %s(%d)\n",
			proc->Security.ExecutionMode == Tasking::User ? "user" : "kernel",
			proc->Name, proc->ID,
			thread->Name, thread->ID);
}
