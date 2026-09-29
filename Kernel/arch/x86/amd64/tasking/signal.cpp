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

#include <signal.hpp>
#include <dumper.hpp>
#include <task.hpp>
#include <errno.h>

#include "../cpu/gdt.hpp"

/* subsystem/linux/syscall.cpp */
extern int ConvertSignalToLinux(signal_t sig);

namespace Tasking
{
	bool Signal::HandleSignal(CPU::SchedulerFrame *tf, void *thread)
	{
		/* We don't want to do this in kernel mode */
		if (tf->cs != GDT_USER_CODE)
			return false;

		if (Queue.empty())
			return false;

		debug("We have %d signals to handle", Queue.size());

		SmartLock(SignalLock);
		SignalInfo sigI = GetAvailableSignal(thread);
		if (sigI.sig == SIGNULL)
			return false;

		uintptr_t _p_rsp = ((PCB *)ctx)->PageTable->Get(tf->rsp);
		uint64_t paRsp = _p_rsp;
		paRsp &= ~0xF; /* Align */
		paRsp -= 128;  /* Red zone */

		/* Calculate the virtual rsp */
		uintptr_t _v_rsp = tf->rsp;
		_v_rsp &= ~0xF; /* Align */
		_v_rsp -= 128;	/* Red zone */
		uint64_t *vRsp = (uint64_t *)(_v_rsp - sizeof(StackInfo));
		vRsp--; /* Alignment */
		vRsp--; /* Handler Address */
		assert(!((uintptr_t)vRsp & 0xF));

		/* Add the stack info */
		StackInfo si{};
		CPU::x86::fxsave(&si.fx);
		si.tf = *tf;
		si.GSBase = CPU::x86::rdmsr(CPU::x86::MSR_GS_BASE);
		si.FSBase = CPU::x86::rdmsr(CPU::x86::MSR_FS_BASE);
		si.ShadowGSBase = CPU::x86::rdmsr(CPU::x86::MSR_SHADOW_GS_BASE);
		si.SignalMask = ((TCB *)thread)->Signals.Mask.to_ulong();
		si.Compatibility = ((PCB *)ctx)->Info.Compatibility;

		debug("gs: %#lx fs: %#lx shadow: %#lx",
			  si.GSBase, si.FSBase, si.ShadowGSBase);

		/* Copy the stack info */
		uint64_t *pRsp = (uint64_t *)(paRsp - sizeof(StackInfo));
		memcpy(pRsp, &si, sizeof(StackInfo));

		/* Set the handler address */
		pRsp--; /* Alignment */
		pRsp--;
		*pRsp = uint64_t(sa[sigI.sig].sa_handler.Handler);

		assert(!((uintptr_t)pRsp & 0xF));

		int cSig = LinuxSig() ? ConvertSignalToLinux((signal_t)sigI.sig) : sigI.sig;

#ifdef DEBUG
		DumpData("Stack Data", (void *)pRsp,
				 paRsp - uint64_t(pRsp));
		debug("initial stack tf->rsp: %#lx after: %#lx",
			  tf->rsp, uint64_t(vRsp));
		debug("sig: %d -> %d", sigI.sig, cSig);
#endif

		tf->rsp = uint64_t(vRsp);
		tf->rip = uint64_t(TrampAddr);

		/* void func(int signo); */
		/* void func(int signo, siginfo_t *info, void *context); */
		tf->rdi = cSig;
		if (sa[sigI.sig].Flags & SA_SIGINFO)
		{
			fixme("SA_SIGINFO not implemented");
			siginfo_t *info = 0;
			void *context = 0;
			tf->rsi = uint64_t(info);
			tf->rdx = uint64_t(context);
			tf->rcx = 0;
			tf->r8 = 0;
			tf->r9 = 0;
		}
		else
		{
			tf->rsi = 0;
			tf->rdx = 0;
			tf->rcx = 0;
			tf->r8 = 0;
			tf->r9 = 0;
		}

		((TCB *)thread)->Signals.Mask = sa[sigI.sig].Mask;
		assert(TrampAddr != nullptr);
		return true;
	}

	void Signal::RestoreHandleSignal(SyscallsFrame *sf, void *thread)
	{
		debug("Restoring signal handler");
		SmartLock(SignalLock);

		gsTCB *gs = (gsTCB *)CPU::x86::rdmsr(CPU::x86::MSR_GS_BASE);
		uint64_t *sp = (uint64_t *)((PCB *)ctx)->PageTable->Get(gs->TempStack);
		sp++; /* Alignment */
		sp++; /* Handler Address */

		assert(!((uintptr_t)sp & 0xF));

		StackInfo *si = (StackInfo *)sp;
		assert(si != nullptr);

		sf->r15 = si->tf.r15;
		sf->r14 = si->tf.r14;
		sf->r13 = si->tf.r13;
		sf->r12 = si->tf.r12;
		sf->r11 = si->tf.r11;
		sf->r10 = si->tf.r10;
		sf->r9 = si->tf.r9;
		sf->r8 = si->tf.r8;
		sf->bp = si->tf.rbp;
		sf->di = si->tf.rdi;
		sf->si = si->tf.rsi;
		sf->dx = si->tf.rdx;
		sf->cx = si->tf.rcx;
		sf->bx = si->tf.rbx;
		sf->ax = si->tf.rax;
		sf->Flags = si->tf.rflags.raw;
		sf->ReturnAddress = si->tf.rip;
		gs->TempStack = (void *)si->tf.rsp;

		((TCB *)thread)->Signals.Mask = si->SignalMask;

		CPU::x86::fxrstor(&si->fx);
		CPU::x86::wrmsr(CPU::x86::MSR_GS_BASE, si->ShadowGSBase);
		CPU::x86::wrmsr(CPU::x86::MSR_FS_BASE, si->FSBase);
		CPU::x86::wrmsr(CPU::x86::MSR_SHADOW_GS_BASE, si->GSBase);
		debug("gs: %#lx fs: %#lx shadow: %#lx",
			  si->GSBase, si->FSBase, si->ShadowGSBase);

		// ((PCB *)ctx)->GetContext()->Yield();
		// __builtin_unreachable();
		/* Return because we will restore at sysretq */
	}
}
