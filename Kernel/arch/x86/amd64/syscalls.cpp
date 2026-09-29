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

#include <syscalls.hpp>

#include <cpu.hpp>

#include "cpu/gdt.hpp"

// https://supercip971.github.io/02-wingos-syscalls.html
using namespace CPU::x64;

// "core/SystemCalls.cpp"
extern "C" uint64_t SystemCallsHandler(SyscallsFrame *regs);

extern "C" void SystemCallHandlerStub();

extern "C" __naked __used __no_stack_protector __aligned(16) void SystemCallHandlerStub()
{
	asmv("swapgs\n");			 /* Swap GS to get the gsTCB */
	asmv("mov %rsp, %gs:0x8\n"); /* We save the current rsp to gsTCB->TempStack */
	asmv("mov %gs:0x0, %rsp\n"); /* Get gsTCB->SystemCallStack and set it as rsp */
	asmv("push $0x1b\n");		 /* Push user data segment for SyscallsFrame */
	asmv("push %gs:0x8\n");		 /* Push gsTCB->TempStack (old rsp) for SyscallsFrame */
	asmv("push %r11\n");		 /* Push the flags for SyscallsFrame */
	asmv("push $0x23\n");		 /* Push user code segment for SyscallsFrame */
	asmv("push %rcx\n");		 /* Push the return address for SyscallsFrame + sysretq (https://www.felixcloutier.com/x86/sysret) */

	/* Push registers */
	asmv("push %rax\n"
		 "push %rbx\n"
		 "push %rcx\n"
		 "push %rdx\n"
		 "push %rsi\n"
		 "push %rdi\n"
		 "push %rbp\n"
		 "push %r8\n"
		 "push %r9\n"
		 "push %r10\n"
		 "push %r11\n"
		 "push %r12\n"
		 "push %r13\n"
		 "push %r14\n"
		 "push %r15\n");

	/* Set the first argument to the SyscallsFrame pointer */
	asmv("mov %rsp, %rdi\n");
	asmv("mov $0, %rbp\n");
	asmv("call SystemCallsHandler\n");

	/* Pop registers except rax */
	asmv("pop %r15\n"
		 "pop %r14\n"
		 "pop %r13\n"
		 "pop %r12\n"
		 "pop %r11\n"
		 "pop %r10\n"
		 "pop %r9\n"
		 "pop %r8\n"
		 "pop %rbp\n"
		 "pop %rdi\n"
		 "pop %rsi\n"
		 "pop %rdx\n"
		 "pop %rcx\n"
		 "pop %rbx\n");

	/* Restore rsp from gsTCB->TempStack */
	asmv("mov %gs:0x8, %rsp\n");
#ifdef DEBUG
	/* Easier to debug stacks */
	asmv("movq $0, %gs:0x8\n");
#endif

	asmv("swapgs\n");  /* Swap GS back to the user GS */
	asmv("sti\n");	   /* Enable interrupts */
	asmv("sysretq\n"); /* Return to rcx address in user mode */
}

void InitializeSystemCalls()
{
	CPU::x86::wrmsr(CPU::x86::MSR_EFER, CPU::x86::rdmsr(CPU::x86::MSR_EFER) | 1);
	CPU::x86::wrmsr(CPU::x86::MSR_STAR, ((uint64_t)(GDT_KERNEL_CODE) << 32) | ((uint64_t)(GDT_KERNEL_DATA | 3) << 48));
	CPU::x86::wrmsr(CPU::x86::MSR_LSTAR, (uint64_t)SystemCallHandlerStub);
	CPU::x86::wrmsr(CPU::x86::MSR_SYSCALL_MASK, (uint64_t)(1 << 9));
}
