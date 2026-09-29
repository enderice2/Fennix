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

#include <scheduler.hpp>

#include <dumper.hpp>
#include <convert.h>
#include <lock.hpp>
#include <printf.h>
#include <smp.hpp>
#include <io.h>

#include "../../kernel.h"

#if defined(__amd64__)
#include "../arch/x86/amd64/cpu/gdt.hpp"
#elif defined(__i386__)
#include "../arch/i386/cpu/gdt.hpp"
#elif defined(__aarch64__)
#endif

// #define DEBUG_SCHEDULER 1
// #define DEBUG_GET_NEXT_AVAILABLE_PROCESS 1
// #define DEBUG_GET_NEXT_AVAILABLE_THREAD 1
// #define DEBUG_FIND_NEW_PROCESS 1
// #define DEBUG_SCHEDULER_SEARCH_PROCESS_THREAD 1
// #define DEBUG_WAKE_UP_THREADS 1

/* Global */
#ifdef DEBUG_SCHEDULER

#define DEBUG_GET_NEXT_AVAILABLE_PROCESS 1
#define DEBUG_GET_NEXT_AVAILABLE_THREAD 1
#define DEBUG_FIND_NEW_PROCESS 1
#define DEBUG_SCHEDULER_SEARCH_PROCESS_THREAD 1
#define DEBUG_WAKE_UP_THREADS 1

#define schedbg(m, ...)      \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define schedbg(m, ...)
#endif

/* GetNextAvailableThread */
#ifdef DEBUG_GET_NEXT_AVAILABLE_PROCESS
#define gnap_schedbg(m, ...) \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define gnap_schedbg(m, ...)
#endif

/* GetNextAvailableProcess */
#ifdef DEBUG_GET_NEXT_AVAILABLE_THREAD
#define gnat_schedbg(m, ...) \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define gnat_schedbg(m, ...)
#endif

/* FindNewProcess */
#ifdef DEBUG_FIND_NEW_PROCESS
#define fnp_schedbg(m, ...)  \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define fnp_schedbg(m, ...)
#endif

/* SchedulerSearchProcessThread */
#ifdef DEBUG_SCHEDULER_SEARCH_PROCESS_THREAD
#define sspt_schedbg(m, ...) \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define sspt_schedbg(m, ...)
#endif

/* WakeUpThreads */
#ifdef DEBUG_WAKE_UP_THREADS
#define wut_schedbg(m, ...)  \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define wut_schedbg(m, ...)
#endif

#if defined(__amd64__) || defined(__i386__)
__naked
#endif
	__used nsa void
	__custom_sched_idle_loop()
{
#if defined(__amd64__) || defined(__i386__)
	asmv("IdleLoop:");
	asmv("hlt");
	asmv("jmp IdleLoop");
#elif defined(__aarch64__)
	asmv("IdleLoop:");
	asmv("wfe");
	asmv("b IdleLoop");
#endif
}

namespace Tasking::Scheduler
{
	bool Custom::RemoveThread(TCB *Thread)
	{
		debug("Thread \"%s\"(%d) removed from process \"%s\"(%d)",
			  Thread->Name, Thread->ID, Thread->Parent->Name,
			  Thread->Parent->ID);

		delete Thread;
		return true;
	}

	bool Custom::RemoveProcess(PCB *Process)
	{
		if (Process->State == Terminated)
		{
			delete Process;
			return true;
		}

		for (TCB *Thread : Process->Threads)
		{
			if (Thread->State == Terminated)
				RemoveThread(Thread);
		}

		return true;
	}

	PCB *Custom::GetProcessByID(TID ID)
	{
		for (auto p : ProcessList)
		{
			if (p->ID == ID)
				return p;
		}
		return nullptr;
	}

	TCB *Custom::GetThreadByID(TID ID, PCB *Parent)
	{
		if (unlikely(Parent == nullptr))
			return nullptr;

		for (auto t : Parent->Threads)
		{
			if (t->ID == ID)
				return t;
		}
		return nullptr;
	}

	void Custom::StartIdleProcess()
	{
		IdleProcess = ctx->GetKernelProcess();
		for (int i = 0; i < SMP::CPUCores; i++)
		{
			TCB *thd = ctx->CreateThread(IdleProcess, IP(__custom_sched_idle_loop));
			char IdleName[16];
			sprintf(IdleName, "Idle Thread %d", i);
			thd->Rename(IdleName);
			thd->SetPriority(Idle);
			for (int j = 0; j < MAX_CPU; j++)
				thd->Info.Affinity[j] = false;
			thd->Info.Affinity[i] = true;

			if (unlikely(i == 0))
				IdleThread = thd;
		}
	}

	std::vector<PCB *> &Custom::GetProcessList()
	{
		return ProcessList;
	}

	void Custom::StartScheduler()
	{
		thisTimer->SetOneShot(std::chrono::milliseconds(Tasking::TaskPriority::Normal), Interrupt::IRQ16);
		// if (Interrupts::apicTimer[0])
		// {
		// 	((APIC::Timer *)Interrupts::apicTimer[0])->OneShot(CPU::x86::IRQ16, 100);

		// 	/* FIXME: The kernel is not ready for multi-core tasking. */
		// 	return;

		// 	APIC::InterruptCommandRegister icr{};
		// 	bool x2APIC = ((APIC::APIC *)Interrupts::apic[0])->x2APIC;

		// 	if (likely(x2APIC))
		// 	{
		// 		icr.x2.VEC = static_cast<uint8_t>(CPU::x86::IRQ16);
		// 		icr.x2.MT = APIC::Fixed;
		// 		icr.x2.L = APIC::Assert;
		// 		icr.x2.DES = 0xFFFFFFFF; /* Broadcast IPI to all local APICs. */
		// 		((APIC::APIC *)Interrupts::apic[0])->ICR(icr);
		// 	}
		// 	else
		// 	{
		// 		icr.VEC = static_cast<uint8_t>(CPU::x86::IRQ16);
		// 		icr.MT = APIC::Fixed;
		// 		icr.L = APIC::Assert;

		// 		for (int i = 0; i < SMP::CPUCores; i++)
		// 		{
		// 			icr.DES = uint8_t(i);
		// 			((APIC::APIC *)Interrupts::apic[i])->ICR(icr);
		// 		}
		// 	}
		// }
	}

	hot void Custom::Yield()
	{
		/* This will trigger the IRQ16
		instantly so we won't execute
		the next instruction */
#if defined(__amd64__) || defined(__i386__)
		asmv("int $0x30");
#elif defined(__aarch64__)
		asmv("svc #0x30");
#endif
	}

	void Custom::PushProcess(PCB *pcb)
	{
		this->ProcessList.push_back(pcb);
	}

	void Custom::PopProcess(PCB *pcb)
	{
		auto it = std::find(this->ProcessList.begin(),
							this->ProcessList.end(), pcb);

		if (it == this->ProcessList.end())
		{
			debug("Process %d not found in the list", pcb->ID);
			return;
		}

		this->ProcessList.erase(it);
	}

	std::pair<PCB *, TCB *> Custom::GetIdle()
	{
		return std::make_pair(IdleProcess, IdleThread);
	}

	/* --------------------------------------------------------------- */

	hot nsa void Custom::OneShot(int TimeSlice)
	{
		if (TimeSlice == 0)
			TimeSlice = Tasking::TaskPriority::Normal;

#ifdef DEBUG
		if (DebuggerIsAttached)
			TimeSlice += 10;
#endif

#if defined(__amd64__) || defined(__i386__)
		// ((APIC::Timer *)Interrupts::apicTimer[GetCurrentCPU()->ID])->OneShot(CPU::x86::IRQ16, TimeSlice);
		thisTimer->SetOneShot(std::chrono::milliseconds(TimeSlice), Interrupt::IRQ16);
#elif defined(__aarch64__)
#endif
	}

	hot nsa void Custom::UpdateUsage(TaskInfo *Info, TaskExecutionMode Mode, int Core)
	{
		UNUSED(Core);
		uint64_t CurrentTime = GlobalClock->Now();
		uint64_t TimePassed = Info->LastUpdateTime - CurrentTime;
		Info->LastUpdateTime = CurrentTime;

		if (Mode == TaskExecutionMode::User)
			Info->UserTime += TimePassed;
		else
			Info->KernelTime += TimePassed;
	}

	hot nsa nif bool Custom::FindNewProcess(void *CPUDataPointer)
	{
		CPUData *CurrentCPU = (CPUData *)CPUDataPointer;
		fnp_schedbg("%d processes", ProcessList.size());
#ifdef DEBUG_FIND_NEW_PROCESS
		for (auto process : ProcessList)
			fnp_schedbg("Process %d %s", process->ID,
						process->Name);
#endif
		for (auto process : ProcessList)
		{
			switch (process->State.load())
			{
			case TaskState::Ready:
				fnp_schedbg("Ready process (%s)%d",
							process->Name, process->ID);
				break;
			default:
				fnp_schedbg("Process \"%s\"(%d) status %d",
							process->Name, process->ID,
							process->State);

				/* We don't actually remove the process. RemoveProcess
				   firstly checks if it's terminated, if not, it will
				   loop through Threads and call RemoveThread on
				   terminated threads. */
				RemoveProcess(process);
				continue;
			}

			for (auto thread : process->Threads)
			{
				if (thread->State.load() != TaskState::Ready)
					continue;

				if (thread->Info.Affinity[CurrentCPU->ID] == false)
					continue;

				CurrentCPU->CurrentProcess = process;
				CurrentCPU->CurrentThread = thread;
				return true;
			}
		}
		fnp_schedbg("No process to run.");
		return false;
	}

	hot nsa nif bool Custom::GetNextAvailableThread(void *CPUDataPointer)
	{
		CPUData *CurrentCPU = (CPUData *)CPUDataPointer;

		size_t ThreadsSize = CurrentCPU->CurrentProcess->Threads.size();

		for (size_t i = 0; i < ThreadsSize; i++)
		{
			if (CurrentCPU->CurrentProcess->Threads[i] == CurrentCPU->CurrentThread.load())
			{
				size_t TempIndex = i;
			RetryAnotherThread:
				if (TempIndex + 1 >= ThreadsSize)
					break;

				TCB *nextThread = CurrentCPU->CurrentProcess->Threads[TempIndex + 1];

				gnat_schedbg("\"%s\"(%d) and next thread is \"%s\"(%d)",
							 CurrentCPU->CurrentProcess->Threads[i]->Name,
							 CurrentCPU->CurrentProcess->Threads[i]->ID,
							 nextThread->Name, nextThread->ID);

				if (nextThread->State.load() != TaskState::Ready)
				{
					gnat_schedbg("Thread %d is not ready", nextThread->ID);
					TempIndex++;
					goto RetryAnotherThread;
				}

				if (nextThread->Info.Affinity[CurrentCPU->ID] == false)
				{
					TempIndex++;
					goto RetryAnotherThread;
				}

				CurrentCPU->CurrentThread = nextThread;
				gnat_schedbg("[thd 0 -> end] Scheduling thread %d parent of %s->%d Procs %d",
							 nextThread->ID, nextThread->Parent->Name,
							 ThreadsSize, ProcessList.size());
				return true;
			}
#ifdef DEBUG
			else
			{
				gnat_schedbg("Thread %d is not the current one",
							 CurrentCPU->CurrentProcess->Threads[i]->ID);
			}
#endif
		}
		return false;
	}

	hot nsa nif bool Custom::GetNextAvailableProcess(void *CPUDataPointer)
	{
		CPUData *CurrentCPU = (CPUData *)CPUDataPointer;

		bool Skip = true;
		for (auto process : ProcessList)
		{
			if (process == CurrentCPU->CurrentProcess.load())
			{
				Skip = false;
				gnap_schedbg("Found current process %#lx", process);
				continue;
			}

			if (Skip)
			{
				gnap_schedbg("Skipping process %#lx", process);
				continue;
			}

			if (process->State.load() != TaskState::Ready)
			{
				gnap_schedbg("Process %d is not ready", process->ID);
				continue;
			}

			for (auto thread : process->Threads)
			{
				if (thread->State.load() != TaskState::Ready)
				{
					gnap_schedbg("Thread %d is not ready", thread->ID);
					continue;
				}

				if (thread->Info.Affinity[CurrentCPU->ID] == false)
					continue;

				CurrentCPU->CurrentProcess = process;
				CurrentCPU->CurrentThread = thread;
				gnap_schedbg("[cur proc+1 -> first thd] Scheduling thread %d %s->%d (Total Procs %d)",
							 thread->ID, thread->Name, process->Threads.size(), ProcessList.size());
				return true;
			}
		}
		gnap_schedbg("No process to run.");
		return false;
	}

	hot nsa nif bool Custom::SchedulerSearchProcessThread(void *CPUDataPointer)
	{
		CPUData *CurrentCPU = (CPUData *)CPUDataPointer;

		for (auto process : ProcessList)
		{
			if (process->State.load() != TaskState::Ready)
			{
				sspt_schedbg("Process %d is not ready", process->ID);
				continue;
			}

			for (auto thread : process->Threads)
			{
				if (thread->State.load() != TaskState::Ready)
				{
					sspt_schedbg("Thread %d is not ready", thread->ID);
					continue;
				}

				if (thread->Info.Affinity[CurrentCPU->ID] == false)
					continue;

				CurrentCPU->CurrentProcess = process;
				CurrentCPU->CurrentThread = thread;
				sspt_schedbg("[proc 0 -> end -> first thd] Scheduling thread %d parent of %s->%d (Procs %d)",
							 thread->ID, thread->Parent->Name, process->Threads.size(), ProcessList.size());
				return true;
			}
		}
		return false;
	}

	nsa nif void Custom::UpdateProcessState()
	{
		for (auto process : ProcessList)
		{
			if (process->State.load() == TaskState::Terminated)
				continue;

			if (process->Threads.size() == 1)
			{
				process->State.exchange(process->Threads.front()->State.load());
				continue;
			}

			bool AllThreadsSleeping = true;
			for (auto thread : process->Threads)
			{
				if (thread->State.load() == TaskState::Terminated)
					continue;

				if (thread->State.load() != TaskState::Sleeping)
				{
					AllThreadsSleeping = false;
					break;
				}
			}

			if (AllThreadsSleeping)
				process->State.store(TaskState::Sleeping);
			else if (process->State.load() == TaskState::Sleeping)
				process->State.store(TaskState::Ready);
		}
	}

	nsa nif void Custom::WakeUpThreads()
	{
		for (auto process : ProcessList)
		{
			Tasking::TaskState pState = process->State.load();
			if (pState != TaskState::Ready &&
				pState != TaskState::Sleeping &&
				pState != TaskState::Blocked)
				continue;

			for (auto thread : process->Threads)
			{
				if (likely(thread->State.load() != TaskState::Sleeping))
					continue;

				/* Check if the thread is ready to wake up. */
				if (unlikely(thread->Info.SleepUntil < GlobalClock->Now()))
				{
					if (pState == TaskState::Sleeping)
						process->State.store(TaskState::Ready);
					thread->State.store(TaskState::Ready);

					thread->Info.SleepUntil = 0;
					wut_schedbg("Thread \"%s\"(%d) woke up.", thread->Name, thread->ID);
				}
				else
				{
					wut_schedbg("Thread \"%s\"(%d) is not ready to wake up. (SleepUntil: %d, Counter: %d)",
								thread->Name, thread->ID, thread->Info.SleepUntil, GlobalClock->Now());
				}
			}
		}
	}

	nsa nif void Custom::CleanupTerminated()
	{
		for (auto pcb : ProcessList)
		{
			if (pcb->State.load() == TaskState::Terminated)
			{
				debug("Found terminated process %s(%d)", pcb->Name, pcb->ID);
				delete pcb;
				continue;
			}

			for (TCB *tcb : pcb->Threads)
			{
				if (tcb->State == Terminated)
					delete tcb;
			}
		}
	}

	hot nsa nif void Custom::Schedule(CPU::SchedulerFrame *Frame)
	{
		if (unlikely(StopScheduler))
		{
			warn("Scheduler stopped.");
			return;
		}
		bool ProcessNotChanged = false;
		uint64_t SchedTmpTicks = GlobalClock->Now();
		this->LastTaskTicks.store(size_t(SchedTmpTicks - this->SchedulerTicks.load()));
		CPUData *CurrentCPU = GetCurrentCPU();
		this->LastCore.store(CurrentCPU->ID);
		schedbg("Scheduler called on CPU %d.", CurrentCPU->ID);

		if (unlikely(!CurrentCPU->CurrentProcess.load() ||
					 !CurrentCPU->CurrentThread.load()))
		{
			schedbg("Invalid process or thread. Finding a new one.");
			ProcessNotChanged = true;
			if (this->FindNewProcess(CurrentCPU))
				goto Success;
			else
				goto Idle;
		}
		else
		{
			CurrentCPU->CurrentThread->Registers = *Frame;
#if defined(__amd64__) || defined(__i386__)
			CPU::x86::fxsave(&CurrentCPU->CurrentThread->FPU);
			CurrentCPU->CurrentThread->ShadowGSBase = CPU::x86::rdmsr(CPU::x86::MSR_SHADOW_GS_BASE);
			CurrentCPU->CurrentThread->GSBase = CPU::x86::rdmsr(CPU::x86::MSR_GS_BASE);
			CurrentCPU->CurrentThread->FSBase = CPU::x86::rdmsr(CPU::x86::MSR_FS_BASE);
#endif

			if (CurrentCPU->CurrentProcess->State.load() == TaskState::Running)
				CurrentCPU->CurrentProcess->State.store(TaskState::Ready);
			if (CurrentCPU->CurrentThread->State.load() == TaskState::Running)
				CurrentCPU->CurrentThread->State.store(TaskState::Ready);

			this->CleanupTerminated();
			schedbg("Passed CleanupTerminated");

			this->UpdateProcessState();
			schedbg("Passed UpdateProcessState");

			this->WakeUpThreads();
			schedbg("Passed WakeUpThreads");

			if (this->SchedulerUpdateTrapFrame)
			{
				debug("Updating trap frame");
				this->SchedulerUpdateTrapFrame = false;
				CurrentCPU->CurrentProcess->State.store(TaskState::Running);
				CurrentCPU->CurrentThread->State.store(TaskState::Running);
				*Frame = CurrentCPU->CurrentThread->Registers;
				this->SchedulerTicks.store(size_t(GlobalClock->Now() - SchedTmpTicks));
				return;
			}

			if (this->GetNextAvailableThread(CurrentCPU))
			{
				ProcessNotChanged = true;
				goto Success;
			}
			schedbg("Passed GetNextAvailableThread");

			if (this->GetNextAvailableProcess(CurrentCPU))
			{
				goto Success;
			}
			schedbg("Passed GetNextAvailableProcess");

			if (SchedulerSearchProcessThread(CurrentCPU))
			{
				schedbg("Passed SchedulerSearchProcessThread");
				goto Success;
			}
			else
			{
				schedbg("SchedulerSearchProcessThread failed. Going idle.");
				goto Idle;
			}
		}

		assert(!"Unwanted code execution");

	Idle:
		ProcessNotChanged = true;
		CurrentCPU->CurrentProcess = IdleProcess;
		CurrentCPU->CurrentThread = IdleThread;

	Success:
		schedbg("Process \"%s\"(%d) Thread \"%s\"(%d) is now running on CPU %d",
				CurrentCPU->CurrentProcess->Name, CurrentCPU->CurrentProcess->ID,
				CurrentCPU->CurrentThread->Name, CurrentCPU->CurrentThread->ID, CurrentCPU->ID);

		if (!ProcessNotChanged)
			UpdateUsage(&CurrentCPU->CurrentProcess->Info,
						CurrentCPU->CurrentProcess->Security.ExecutionMode,
						CurrentCPU->ID);

		UpdateUsage(&CurrentCPU->CurrentThread->Info,
					CurrentCPU->CurrentThread->Security.ExecutionMode,
					CurrentCPU->ID);

		CurrentCPU->CurrentProcess->State.store(TaskState::Running);
		CurrentCPU->CurrentThread->State.store(TaskState::Running);

#if defined(__amd64__) || defined(__i386__)
		if (CurrentCPU->CurrentThread->Registers.cs != GDT_KERNEL_CODE)
			CurrentCPU->CurrentThread->Registers.ppt = (uint64_t)(void *)CurrentCPU->CurrentProcess->PageTable;
		else
			CurrentCPU->CurrentThread->Registers.ppt = (uint64_t)(void *)KernelPageTable;
#endif

		// if (!SchedulerUpdateTrapFrame) {} // TODO

		*Frame = CurrentCPU->CurrentThread->Registers;

#if defined(__amd64__) || defined(__i386__)
		GlobalDescriptorTable::SetKernelStack((void *)((uintptr_t)CurrentCPU->CurrentThread->Stack->GetStackTop()));
		CPU::x86::fxrstor(&CurrentCPU->CurrentThread->FPU);
		CPU::x86::wrmsr(CPU::x86::MSR_SHADOW_GS_BASE, CurrentCPU->CurrentThread->ShadowGSBase);
		CPU::x86::wrmsr(CPU::x86::MSR_GS_BASE, CurrentCPU->CurrentThread->GSBase);
		CPU::x86::wrmsr(CPU::x86::MSR_FS_BASE, CurrentCPU->CurrentThread->FSBase);
#endif

		CurrentCPU->CurrentProcess->Signals.HandleSignal(Frame, CurrentCPU->CurrentThread.load());

		if (!ProcessNotChanged)
			(&CurrentCPU->CurrentProcess->Info)->LastUpdateTime = GlobalClock->Now();
		(&CurrentCPU->CurrentThread->Info)->LastUpdateTime = GlobalClock->Now();
		this->OneShot(CurrentCPU->CurrentThread->Info.Priority);

		if (CurrentCPU->CurrentThread->Security.IsDebugEnabled &&
			CurrentCPU->CurrentThread->Security.IsKernelDebugEnabled)
		{
#ifdef __amd64__
			trace("%s[%ld]: RIP=%#lx  RBP=%#lx  RSP=%#lx",
				  CurrentCPU->CurrentThread->Name, CurrentCPU->CurrentThread->ID,
				  CurrentCPU->CurrentThread->Registers.rip,
				  CurrentCPU->CurrentThread->Registers.rbp,
				  CurrentCPU->CurrentThread->Registers.rsp);
#elif defined(__i386__)
			trace("%s[%ld]: EIP=%#lx  EBP=%#lx  ESP=%#lx",
				  CurrentCPU->CurrentThread->Name, CurrentCPU->CurrentThread->ID,
				  CurrentCPU->CurrentThread->Registers.eip,
				  CurrentCPU->CurrentThread->Registers.ebp,
				  CurrentCPU->CurrentThread->Registers.esp);
#elif defined(__aarch64__)
#warning "aarch64 not implemented yet"
#endif
		}

		this->SchedulerTicks.store(size_t(GlobalClock->Now() - SchedTmpTicks));
	}

	hot nsa nif int Custom::OnInterruptReceived(CPU::SchedulerFrame *Frame)
	{
		SmartCriticalSection(SchedulerLock);
		this->Schedule(Frame);
		return EOK;
	}

	Custom::Custom(Task *ctx) : Base(ctx), Interrupt::Handler(16) /* IRQ16 */
	{
#if defined(__amd64__) || defined(__i386__)
		// Map the IRQ16 to the first CPU.
		// ((APIC::APIC *)Interrupts::apic[0])->RedirectIRQ(0, CPU::x86::IRQ16 - CPU::x86::IRQ0, 1);
#endif
	}

	Custom::~Custom()
	{
		for (PCB *Process : ProcessList)
		{
			for (TCB *Thread : Process->Threads)
			{
				if (Thread == GetCurrentCPU()->CurrentThread.load())
					continue;
				ctx->KillThread(Thread, KILL_SCHEDULER_DESTRUCTION);
			}

			if (Process == GetCurrentCPU()->CurrentProcess.load())
				continue;
			ctx->KillProcess(Process, KILL_SCHEDULER_DESTRUCTION);
		}

		debug("Waiting for processes to terminate");
		uint64_t timeout = GlobalClock->Now() + std::chrono::seconds(20).count();
		while (this->GetProcessList().size() > 0)
		{
			trace("Waiting for %d processes to terminate", this->GetProcessList().size());
			int NotTerminated = 0;
			for (PCB *Process : this->GetProcessList())
			{
				trace("Process %s(%d) is still running (or waiting to be removed state %#lx)",
					  Process->Name, Process->ID, Process->State);

				if (Process->State == TaskState::Terminated)
				{
					debug("Process %s(%d) terminated", Process->Name, Process->ID);
					continue;
				}

				NotTerminated++;
			}
			if (NotTerminated == 1)
				break;

			ctx->Sleep(1000);
			debug("Current working process is %s(%d)", ctx->GetCurrentProcess()->Name, ctx->GetCurrentProcess()->ID);

			if (GlobalClock->Now() > timeout)
			{
				error("Timeout waiting for processes to terminate");
				break;
			}

			this->OneShot(100);
		}
	}
}
