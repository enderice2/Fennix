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

#include <task.hpp>

#include <scheduler.hpp>
#include <dumper.hpp>
#include <convert.h>
#include <lock.hpp>
#include <printf.h>
#include <smp.hpp>
#include <io.h>

#include "../kernel.h"

#if defined(__amd64__)
#include "../arch/x86/amd64/cpu/gdt.hpp"
#elif defined(__i386__)
#include "../arch/i386/cpu/apic.hpp"
#elif defined(__aarch64__)
#endif

// #define DEBUG_TASKING 1

#ifdef DEBUG_TASKING
#define tskdbg(m, ...)       \
	debug(m, ##__VA_ARGS__); \
	__sync
#else
#define tskdbg(m, ...)
#endif

namespace Tasking
{
	PCB *Task::GetCurrentProcess()
	{
		return GetCurrentCPU()->CurrentProcess.load();
	}

	TCB *Task::GetCurrentThread()
	{
		return GetCurrentCPU()->CurrentThread.load();
	}

	PCB *Task::GetProcessByID(TID ID)
	{
		return ((Scheduler::Base *)Scheduler)->GetProcessByID(ID);
	}

	TCB *Task::GetThreadByID(TID ID, PCB *Parent)
	{
		return ((Scheduler::Base *)Scheduler)->GetThreadByID(ID, Parent);
	}

	std::vector<PCB *> Task::GetProcessList()
	{
		return ((Scheduler::Base *)Scheduler)->GetProcessList();
	}

	void Task::Panic()
	{
		((Scheduler::Base *)Scheduler)->StopScheduler.store(true);
	}

	bool Task::IsPanic()
	{
		return ((Scheduler::Base *)Scheduler)->StopScheduler;
	}

	void Task::Yield()
	{
		((Scheduler::Base *)Scheduler)->Yield();
	}

	void Task::UpdateFrame()
	{
		((Scheduler::Base *)Scheduler)->SchedulerUpdateTrapFrame = true;
		((Scheduler::Base *)Scheduler)->Yield();
	}

	void Task::PushProcess(PCB *pcb)
	{
		((Scheduler::Base *)Scheduler)->PushProcess(pcb);
	}

	void Task::PopProcess(PCB *pcb)
	{
		((Scheduler::Base *)Scheduler)->PopProcess(pcb);
	}

	void Task::WaitForProcess(PCB *pcb)
	{
		if (pcb == nullptr)
			return;

		if (pcb->State == TaskState::UnknownStatus)
			return;

		debug("Waiting for process \"%s\"(%d)",
			  pcb->Name, pcb->ID);

		while (pcb->State != TaskState::Terminated &&
			   pcb->State != TaskState::Zombie &&
			   pcb->State != TaskState::CoreDump)
			this->Yield();
	}

	void Task::WaitForThread(TCB *tcb)
	{
		if (tcb == nullptr)
			return;

		if (tcb->State == TaskState::UnknownStatus)
			return;

		debug("Waiting for thread \"%s\"(%d)",
			  tcb->Name, tcb->ID);

		while (tcb->State != TaskState::Terminated &&
			   tcb->State != TaskState::Zombie &&
			   tcb->State != TaskState::CoreDump)
			this->Yield();
	}

	void Task::WaitForProcessStatus(PCB *pcb, TaskState status)
	{
		if (pcb == nullptr)
			return;

		if (pcb->State == TaskState::UnknownStatus)
			return;

		debug("Waiting for process \"%s\"(%d) to reach status: %d",
			  pcb->Name, pcb->ID, status);

		while (pcb->State != status)
			this->Yield();
	}

	void Task::WaitForThreadStatus(TCB *tcb, TaskState status)
	{
		if (tcb == nullptr)
			return;

		if (tcb->State == TaskState::UnknownStatus)
			return;

		debug("Waiting for thread \"%s\"(%d) to reach status: %d",
			  tcb->Name, tcb->ID, status);

		while (tcb->State != status)
			this->Yield();
	}

	void Task::Sleep(uint64_t Nanoseconds, bool NoSwitch)
	{
		TCB *thread = this->GetCurrentThread();
		PCB *process = thread->Parent;

		thread->SetState(TaskState::Sleeping);

		{
			SmartLock(TaskingLock);
			if (process->Threads.size() == 1)
				process->SetState(TaskState::Sleeping);

			thread->Info.SleepUntil = GlobalClock->Now() + Nanoseconds;
		}

		// #ifdef DEBUG
		// 		uint64_t TicksNow = GlobalClock->Now();
		// #endif
		// 		debug("Thread \"%s\"(%d) is going to sleep until %llu, current %llu, diff %llu",
		// 			  thread->Name, thread->ID, thread->Info.SleepUntil,
		// 			  TicksNow, thread->Info.SleepUntil - TicksNow);

		if (!NoSwitch)
			this->Yield();
	}

	void Task::SignalShutdown()
	{
		debug("Current process is %s(%d) and thread is %s(%d)",
			  GetCurrentProcess()->Name, GetCurrentProcess()->ID,
			  GetCurrentThread()->Name, GetCurrentThread()->ID);

		for (auto pcb : ((Scheduler::Base *)Scheduler)->GetProcessList())
		{
			if (pcb->State == TaskState::Terminated ||
				pcb->State == TaskState::Zombie)
				continue;

			if (pcb == GetCurrentProcess())
				continue;

			debug("Sending SIGTERM to process \"%s\"(%d)",
				  pcb->Name, pcb->ID);
			pcb->SendSignal(SIGTERM);
		}

		// TODO: wait for processes to terminate with timeout.
	}

	__no_sanitize("undefined")
		TCB *Task::CreateThread(PCB *Parent, IP EntryPoint,
								const char **argv, const char **envp,
								const std::vector<AuxiliaryVector> &auxv,
								TaskArchitecture _arch, TaskCompatibility Compatibility,
								bool ThreadNotReady)
	{
		SmartLock(TaskingLock);
		return new TCB(this, Parent, EntryPoint,
					   argv, envp, auxv, _arch,
					   Compatibility, ThreadNotReady);
	}

	PCB *Task::CreateProcess(PCB *Parent, const char *Name,
							 TaskExecutionMode ExecutionMode, bool UseKernelPageTable,
							 uint16_t UserID, uint16_t GroupID)
	{
		SmartLock(TaskingLock);
		return new PCB(this, Parent, Name, ExecutionMode,
					   UseKernelPageTable, UserID, GroupID);
	}

	void Task::StartScheduler()
	{
		((Scheduler::Base *)Scheduler)->StartScheduler();
		debug("Tasking Started");
	}

	struct TaskNode : public Inode
	{
		kstat Stat;
		Inode *Parent;
		std::string Name;
		std::vector<TaskNode *> Children;
	};

	int __task_Lookup(struct Inode *Parent, const char *Name, struct Inode **Result)
	{
		return -ENOSYS;
	}

	int __task_Create(struct Inode *Parent, const char *Name, mode_t Mode, struct Inode **Result)
	{
		TaskNode *p = (TaskNode *)Parent;

		if (!S_ISDIR(p->Mode))
			return -ENOTDIR;

		TaskNode *newNode = new TaskNode;
		newNode->Parent = p;
		newNode->Name = Name;
		newNode->Mode = Mode;
		p->Children.push_back(newNode);
		*Result = newNode;
		return 0;
	}

	ssize_t __task_Read(struct Inode *Node, void *Buffer, size_t Size, off_t Offset)
	{
		return -ENOSYS;
	}

	ssize_t __task_Write(struct Inode *Node, const void *Buffer, size_t Size, off_t Offset)
	{
		return -ENOSYS;
	}

	__no_sanitize("alignment") ssize_t __task_Readdir(struct Inode *_Node, struct kdirent *Buffer, size_t Size, off_t Offset, off_t Entries)
	{
		auto node = (TaskNode *)_Node;
		off_t realOffset = Offset;
		size_t totalSize = 0;
		uint16_t reclen = 0;
		struct kdirent *ent = nullptr;

		if (!S_ISDIR(node->Mode))
			return -ENOTDIR;

		if (Offset == 0)
		{
			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen(".") + 1);
			if (totalSize + reclen > Size)
				return -EINVAL;
			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
			ent->d_ino = node->Index;
			ent->d_off = 0;
			ent->d_reclen = reclen;
			ent->d_type = DT_DIR;
			strcpy(ent->d_name, ".");
			totalSize += reclen;
			Offset++;
		}

		if (Offset == 1)
		{
			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen("..") + 1);
			if (totalSize + reclen > Size)
				return totalSize;

			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
			ent->d_ino = node->Parent ? node->Parent->Index : 0;
			ent->d_off = 1;
			ent->d_reclen = reclen;
			ent->d_type = DT_DIR;
			strcpy(ent->d_name, "..");
			totalSize += reclen;
			Offset++;
		}

		off_t entryIndex = 0;
		for (const auto &var : node->Children)
		{
			if (entryIndex + 2 < realOffset)
			{
				entryIndex++;
				continue;
			}
			if (Entries && entryIndex >= Entries)
				break;

			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen(var->Name.c_str()) + 1);
			if (totalSize + reclen > Size)
				break;
			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
			ent->d_ino = var->Index;
			ent->d_off = entryIndex + 2;
			ent->d_reclen = reclen;
			ent->d_type = IFTODT(var->Mode);
			strcpy(ent->d_name, var->Name.c_str());
			totalSize += reclen;
			entryIndex++;
		}

		if (totalSize + offsetof(struct kdirent, d_name) + 1 > Size)
			return totalSize;

		ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
		ent->d_ino = 0;
		ent->d_off = 0;
		ent->d_reclen = 0;
		ent->d_type = DT_UNKNOWN;
		ent->d_name[0] = '\0';
		return totalSize;
	}

	int __task_SymLink(Inode *Parent, const char *Name, const char *Target, Inode **Result)
	{
		return -ENOSYS;
	}

	ssize_t __task_ReadLink(Inode *Node, char *Buffer, size_t Size)
	{
		switch (Node->GetMajor())
		{
		case 0:
		{
			switch (Node->GetMinor())
			{
			case 0:
			{
				/* FIXME: https://github.com/torvalds/linux/blob/c942a0cd3603e34dd2d7237e064d9318cb7f9654/fs/proc/self.c#L11
						https://lxr.linux.no/#linux+v3.2.9/fs/proc/base.c#L2482 */

				int ret = snprintf(Buffer, Size, "/proc/%d", thisProcess->ID);
				debug("ReadLink: %s (%d bytes)", Buffer, ret);
				return ret;
			}
			default:
				return -ENOENT;
			}
		}
		default:
			return -ENOENT;
		}
	}

	int __task_Stat(struct Inode *Node, kstat *Stat)
	{
		TaskNode *node = (TaskNode *)Node;
		*Stat = node->Stat;
		return 0;
	}

	int __task_AllocateInode(struct FileSystemInfo *, struct Inode **Result)
	{
		TaskNode *ret = new TaskNode;
		*Result = (Inode *)ret;
		return 0;
	}

	int __task_DeleteInode(struct FileSystemInfo *, struct Inode *Node)
	{
		delete Node;
		return 0;
	}

	Task::Task(const IP EntryPoint)
	{
		Node root = fs->GetRoot(0);
		FileSystemInfo *fsi = new FileSystemInfo;
		fsi->Name = "procfs";
		fsi->SuperOps.AllocateInode = __task_AllocateInode;
		fsi->SuperOps.DeleteInode = __task_DeleteInode;
		fsi->Ops.Lookup = __task_Lookup;
		fsi->Ops.Create = __task_Create;
		fsi->Ops.Read = __task_Read;
		fsi->Ops.Write = __task_Write;
		fsi->Ops.ReadDir = __task_Readdir;
		fsi->Ops.SymLink = __task_SymLink;
		fsi->Ops.ReadLink = __task_ReadLink;
		fsi->Ops.Stat = __task_Stat;

		/* d rwx rwx rwx */
		mode_t mode = S_IRWXU |
					  S_IRWXG |
					  S_IRWXO |
					  S_IFDIR;
		TaskNode *inode = new TaskNode;
		inode->Device = fs->RegisterFileSystem(fsi);
		inode->Mode = mode;
		Node proc = fs->Mount(root, inode, "proc", fsi);
		// proc->fsi = fsi;
		// inode->Parent = root->inode;
		// inode->Name = "proc";

		/* l rwx rwx rwx */
		mode = S_IRWXU |
			   S_IRWXG |
			   S_IRWXO |
			   S_IFLNK;
		Node self = fs->Create(proc, "self", mode);
		self->inode->Device = inode->Device;
		self->inode->SetDevice(0, 0);

		/* I don't know if this is the best way to do this. */
		Scheduler::Custom *custom_sched = new Scheduler::Custom(this);
		Scheduler::Base *sched = reinterpret_cast<Scheduler::Base *>(custom_sched);
		__sched_ctx = custom_sched;
		Scheduler = sched;

		KernelProcess = CreateProcess(nullptr, "Kernel", Kernel, true, 0, 0);
		TCB *kthrd = CreateThread(KernelProcess, EntryPoint, nullptr, nullptr, {}, GetKArch());
		kthrd->Rename("Main Thread");
		debug("Created Kernel Process: %s and Thread: %s", KernelProcess->Name, kthrd->Name);

		if (!CPU::Interrupts(CPU::Check))
		{
			error("Interrupts are not enabled.");
			CPU::Interrupts(CPU::Enable);
		}

		((Scheduler::Base *)Scheduler)->StartIdleProcess();
		debug("Tasking is ready");
	}

	Task::~Task()
	{
		delete (Scheduler::Custom *)__sched_ctx;
	}
}
