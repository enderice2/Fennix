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
#include <printf.h>
#include <debug.h>
#include <smp.hpp>
#include <cpu.hpp>

#if defined(__amd64__)
#include "../../arch/x86/amd64/cpu/gdt.hpp"
#elif defined(__i386__)
#elif defined(__aarch64__)
#endif

#include "../../kernel.h"

#ifdef DEBUG
nsa void dbgPrint(CPU::ExceptionFrame *Frame)
{
#if defined(__amd64__)
	debug("FS=%#lx GS=%#lx SS=%#lx CS=%#lx DS=%#lx", Frame->fs, Frame->gs, Frame->ss, Frame->cs, Frame->ds);
	debug("R8=%#lx R9=%#lx R10=%#lx R11=%#lx", Frame->r8, Frame->r9, Frame->r10, Frame->r11);
	debug("R12=%#lx R13=%#lx R14=%#lx R15=%#lx", Frame->r12, Frame->r13, Frame->r14, Frame->r15);
	debug("RAX=%#lx RBX=%#lx RCX=%#lx RDX=%#lx", Frame->rax, Frame->rbx, Frame->rcx, Frame->rdx);
	debug("RSI=%#lx RDI=%#lx RBP=%#lx RSP=%#lx", Frame->rsi, Frame->rdi, Frame->rbp, Frame->rsp);
	debug("RIP=%#lx RFL=%#lx INT=%#lx ERR=%#lx", Frame->rip, Frame->rflags.raw, Frame->InterruptNumber, Frame->ErrorCode);
#elif defined(__i386__)
	debug("FS=%#x GS=%#x CS=%#x DS=%#x", Frame->fs, Frame->gs, Frame->cs, Frame->ds);
	debug("EAX=%#x EBX=%#x ECX=%#x EDX=%#x", Frame->eax, Frame->ebx, Frame->ecx, Frame->edx);
	debug("ESI=%#x EDI=%#x EBP=%#x ESP=%#x", Frame->esi, Frame->edi, Frame->ebp, Frame->esp);
	debug("EIP=%#x EFL=%#x INT=%#x ERR=%#x", Frame->eip, Frame->eflags.raw, Frame->InterruptNumber, Frame->ErrorCode);
#elif defined(__aarch64__)
#endif

#if defined(__amd64__) || defined(__i386__)
	debug("CR2=%#lx CR3=%#lx", Frame->cr2, Frame->cr3);
#endif // defined(__amd64__) || defined(__i386__)

#if defined(__amd64__)
	debug("RFL: CF:%s PF:%s AF:%s ZF:%s SF:%s TF:%s IF:%s DF:%s OF:%s IOPL:%s NT:%s RF:%s VM:%s AC:%s VIF:%s VIP:%s ID:%s AlwaysOne:%d R0:%#x R1:%#x R2:%#x R3:%#x",
		  Frame->rflags.CF ? "True " : "False", Frame->rflags.PF ? "True " : "False", Frame->rflags.AF ? "True " : "False", Frame->rflags.ZF ? "True " : "False",
		  Frame->rflags.SF ? "True " : "False", Frame->rflags.TF ? "True " : "False", Frame->rflags.IF ? "True " : "False", Frame->rflags.DF ? "True " : "False",
		  Frame->rflags.OF ? "True " : "False", Frame->rflags.IOPL ? "True " : "False", Frame->rflags.NT ? "True " : "False", Frame->rflags.RF ? "True " : "False",
		  Frame->rflags.VM ? "True " : "False", Frame->rflags.AC ? "True " : "False", Frame->rflags.VIF ? "True " : "False", Frame->rflags.VIP ? "True " : "False",
		  Frame->rflags.ID ? "True " : "False", Frame->rflags.AlwaysOne,
		  Frame->rflags.Reserved0, Frame->rflags.Reserved1, Frame->rflags.Reserved2, Frame->rflags.Reserved3);
#elif defined(__i386__)
	debug("EFL: CF:%s PF:%s AF:%s ZF:%s SF:%s TF:%s IF:%s DF:%s OF:%s IOPL:%s NT:%s RF:%s VM:%s AC:%s VIF:%s VIP:%s ID:%s AlwaysOne:%d R0:%#x R1:%#x R2:%#x",
		  Frame->eflags.CF ? "True " : "False", Frame->eflags.PF ? "True " : "False", Frame->eflags.AF ? "True " : "False", Frame->eflags.ZF ? "True " : "False",
		  Frame->eflags.SF ? "True " : "False", Frame->eflags.TF ? "True " : "False", Frame->eflags.IF ? "True " : "False", Frame->eflags.DF ? "True " : "False",
		  Frame->eflags.OF ? "True " : "False", Frame->eflags.IOPL ? "True " : "False", Frame->eflags.NT ? "True " : "False", Frame->eflags.RF ? "True " : "False",
		  Frame->eflags.VM ? "True " : "False", Frame->eflags.AC ? "True " : "False", Frame->eflags.VIF ? "True " : "False", Frame->eflags.VIP ? "True " : "False",
		  Frame->eflags.ID ? "True " : "False", Frame->eflags.AlwaysOne,
		  Frame->eflags.Reserved0, Frame->eflags.Reserved1, Frame->eflags.Reserved2);
#elif defined(__aarch64__)
#endif
}
#endif

nsa bool UserModeExceptionHandler(CPU::ExceptionFrame *Frame)
{
	CPUData *core = GetCurrentCPU();
	Tasking::PCB *proc = core->CurrentProcess;
	Tasking::TCB *thread = core->CurrentThread;
	debug("Current process %s(%d) and thread %s(%d)",
		  proc->Name, proc->ID, thread->Name, thread->ID);
	thread->SetState(Tasking::Waiting);

#ifdef DEBUG
	dbgPrint(Frame);
#endif

	int sigRet = -1;
#if defined(__amd64__) || defined(__i386__)
	switch (Frame->InterruptNumber)
	{
	case CPU::x86::PageFault:
	{
		bool Handled = proc->vma->HandleCoW(Frame->cr2);
		if (!Handled)
			Handled = thread->Stack->Expand(Frame->cr2);

		if (likely(Handled))
		{
			debug("Page fault handled");
			Frame->cr2 = 0;
			thread->SetState(Tasking::Ready);
			return true;
		}

		sigRet = proc->Signals.SendSignal(SIGSEGV,
										  {Tasking::KILL_CRASH});
		break;
	}
	case CPU::x86::Debug:
	case CPU::x86::Breakpoint:
	{
		sigRet = proc->Signals.SendSignal(SIGTRAP,
										  {Tasking::KILL_CRASH});
		break;
	}
	case CPU::x86::DivideByZero:
	case CPU::x86::Overflow:
	case CPU::x86::BoundRange:
	case CPU::x86::x87FloatingPoint:
	case CPU::x86::SIMDFloatingPoint:
	{
		sigRet = proc->Signals.SendSignal(SIGFPE,
										  {Tasking::KILL_CRASH});
		break;
	}
	case CPU::x86::InvalidOpcode:
	case CPU::x86::GeneralProtectionFault:
	{
		sigRet = proc->Signals.SendSignal(SIGILL,
										  {Tasking::KILL_CRASH});
		break;
	}
	case CPU::x86::DeviceNotAvailable:
	{
		sigRet = proc->Signals.SendSignal(SIGBUS,
										  {Tasking::KILL_CRASH});
		break;
	}
	case CPU::x86::NonMaskableInterrupt:
	case CPU::x86::DoubleFault:
	case CPU::x86::CoprocessorSegmentOverrun:
	case CPU::x86::InvalidTSS:
	case CPU::x86::SegmentNotPresent:
	case CPU::x86::StackSegmentFault:
	case CPU::x86::AlignmentCheck:
	case CPU::x86::MachineCheck:
	case CPU::x86::Virtualization:
	case CPU::x86::Security:
	default:
	{
		error("Unhandled exception %d on CPU %d",
			  Frame->InterruptNumber, core->ID);
		break;
	}
	}
#else
	UNUSED(proc);
#endif

	if (sigRet == 0)
	{
		trace("User mode exception handler handled");
		thread->SetState(Tasking::Ready);
		return true;
	}

	error("User mode exception handler failed");
	return false;
}
