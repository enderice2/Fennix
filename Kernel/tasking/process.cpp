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

#include <dumper.hpp>
#include <signal.hpp>
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

using namespace vfs;

namespace Tasking
{
	TCB *PCB::GetThread(TID ID)
	{
		auto it = std::find_if(this->Threads.begin(), this->Threads.end(),
							   [ID](TCB *t)
							   { return t->ID == ID; });
		return it != this->Threads.end() ? *it : nullptr;
	}

	int PCB::SendSignal(int sig)
	{
		return this->Signals.SendSignal((signal_t)sig);
	}

	void PCB::SetState(TaskState state)
	{
		this->State.store(state);
		if (this->Threads.size() == 1)
			this->Threads.front()->State.store(state);
	}

	void PCB::SetExitCode(int code)
	{
		this->ExitCode.store(code);
		if (this->Threads.size() == 1)
			this->Threads.front()->ExitCode.store(code);
	}

	void PCB::Rename(const char *name)
	{
		assert(name != nullptr);
		assert(strlen(name) > 0);

		trace("Renaming thread %s to %s",
			  this->Name, name);

		if (this->Name)
		{
			this->AllocatedMemory -= strlen(this->Name) + 1;
			delete[] this->Name;
		}

		this->Name = new char[strlen(name) + 1];
		this->AllocatedMemory += strlen(name) + 1;
		strcpy((char *)this->Name, name);
	}

	void PCB::SetWorkingDirectory(Node node)
	{
		trace("Setting working directory of process %s", node->Name.c_str());
		this->CWD = node;
		Node cwd = fs->Lookup(ProcDirectory, "cwd");
		if (cwd)
			fs->Remove(cwd);
		cwd = fs->CreateLink(ProcDirectory, "cwd", node);
		if (cwd == nullptr)
			error("Failed to create cwd link");
	}

	void PCB::SetExe(const char *path)
	{
		trace("Setting exe %s to %s", this->Name, path);

		Executable = fs->Lookup(ProcDirectory, path);
		if (Executable->IsSymbolicLink())
		{
			char buffer[512];
			fs->ReadLink(Executable, buffer, sizeof(buffer));
			Executable = fs->Lookup(Executable->Parent, buffer);
		}

		Node exe = fs->Lookup(ProcDirectory, "exe");
		if (exe)
			fs->Remove(exe);
		exe = fs->CreateLink(ProcDirectory, "exe", path);
		if (exe == nullptr)
			error("Failed to create exe link");
	}

	size_t PCB::GetSize()
	{
		size_t ret = this->AllocatedMemory;
		ret += this->vma->GetAllocatedMemorySize();

		for (size_t i = 0; i < this->Threads.size(); i++)
			ret += sizeof(TCB);

		for (size_t i = 0; i < this->Children.size(); i++)
			ret += sizeof(PCB);

		return ret;
	}

	PCB::PCB(Task *ctx, PCB *Parent, const char *Name,
			 TaskExecutionMode ExecutionMode,
			 bool UseKernelPageTable,
			 uint16_t UserID, uint16_t GroupID)
		: Signals(this)
	{
		debug("+ %#lx", this);

		assert(ctx != nullptr);
		assert(Name != nullptr);
		assert(strlen(Name) > 0);
		assert(ExecutionMode >= _ExecuteModeMin);
		assert(ExecutionMode <= _ExecuteModeMax);

		Node root = fs->GetRoot(0);
		Node procDir = fs->Lookup(root, "/proc");
		assert(procDir != nullptr);

		/* d r-x r-x r-x */
		mode_t mode = S_IROTH | S_IXOTH |
					  S_IRGRP | S_IXGRP |
					  S_IRUSR | S_IXUSR |
					  S_IFDIR;

		ProcDirectory = fs->Create(procDir, std::to_string(ctx->NextPID).c_str(), mode);
		assert(ProcDirectory != nullptr);

		this->ctx = ctx;
		this->ID = ctx->NextPID++;

		if (this->Name) /* Prevent memory leak */
			delete[] this->Name;

		this->Name = new char[strlen(Name) + 1];
		strcpy((char *)this->Name, Name);
		this->ExitCode = KILL_CRASH;

		/* Check parent */
		if (Parent == nullptr)
			this->Parent = ctx->GetCurrentProcess();
		else
			this->Parent = Parent;

		/* Set uid & gid */
		if (this->Parent &&
			UserID == UINT16_MAX &&
			GroupID == UINT16_MAX)
		{
			UserID = this->Parent->Security.Real.UserID;
			GroupID = this->Parent->Security.Real.GroupID;
			debug("Inherited uid & gid from parent process %s(%d) with uid %d and gid %d",
				  this->Parent->Name, this->Parent->ID, UserID, GroupID);
		}

		this->Security.Real.UserID = UserID;
		this->Security.Real.GroupID = GroupID;
		this->Security.Effective.UserID = UserID;
		this->Security.Effective.GroupID = GroupID;
		this->Security.ExecutionMode = ExecutionMode;

		switch (ExecutionMode)
		{
		case TaskExecutionMode::System:
			fixme("Mode not supported.");
			[[fallthrough]];
		case TaskExecutionMode::Kernel:
		{
			this->Security.IsCritical = true;
			break;
		}
		case TaskExecutionMode::User:
		{
			break;
		}
		default:
			assert(false);
		}

		this->FileDescriptors = new FileDescriptorTable(this);
		this->tty = nullptr;

		/* If create page table */
		if (UseKernelPageTable == false)
		{
			OwnPageTable = true;
			this->PageTable = KernelPageTable->Fork();
			debug("Process %s(%d) has page table at %#lx",
				  this->Name, this->ID, this->PageTable);
		}
		else
			this->PageTable = KernelPageTable;

		this->vma = new Memory::VirtualMemoryArea(this->PageTable);
		this->ProgramBreak = new Memory::ProgramBreak(this->PageTable, this->vma);

		debug("Process page table: %#lx", this->PageTable);
		debug("Created %s process \"%s\"(%d). Parent \"%s\"(%d)",
			  ExecutionMode == TaskExecutionMode::User ? "user" : "kernel",
			  this->Name, this->ID,
			  Parent ? this->Parent->Name : "None",
			  Parent ? this->Parent->ID : 0);

		this->AllocatedMemory += strlen(Name) + 1;
		this->AllocatedMemory += sizeof(PCB);
		this->AllocatedMemory += sizeof(FileDescriptorTable);
		this->AllocatedMemory += FROM_PAGES(TO_PAGES(sizeof(Memory::PageTable) + 1));
		this->AllocatedMemory += sizeof(Memory::VirtualMemoryArea);
		this->AllocatedMemory += sizeof(Memory::ProgramBreak);
		this->AllocatedMemory += sizeof(SymbolResolver::Symbols);

		this->Info.SpawnTime = GlobalClock->Now();

		if (Parent)
			Parent->Children.push_back(this);
		ctx->PushProcess(this);
	}

	PCB::~PCB()
	{
		debug("- %#lx", this);
		debug("Destroying process \"%s\"(%d)",
			  this->Name, this->ID);

		debug("Removing from process list");
		/* Remove us from the process list so we
			don't get scheduled anymore */
		ctx->PopProcess(this);

		debug("Freeing allocated memory");
		delete this->ProgramBreak;
		delete this->vma;

		debug("Closing file descriptors");
		delete this->FileDescriptors;

		debug("Deleting tty");
		// if (this->tty)
		// 	delete ((TTY::TeletypeDriver *)this->tty);
		fixme("remove workarounds for stdio and tty");

		/* FIXME: DON'T DELETE THE TTY
			spawn.cpp is using this as workaround
		 tty == KernelConsole::CurrentTerminal.load()->Term;
		 */

		/* If we own the pointer to the
			PageTable, we need to free it */
		if (this->PageTable && OwnPageTable)
		{
			debug("Freeing page table");
			size_t PTPgs = TO_PAGES(sizeof(Memory::PageTable) + 1);
			KernelAllocator.FreePages(this->PageTable, PTPgs);
		}

		/* Exit all children processes */
		for (auto pcb : this->Children)
		{
			if (pcb == nullptr)
			{
				warn("Process is null? Kernel bug");
				continue;
			}

			debug("Destroying child process \"%s\"(%d)",
				  pcb->Name, pcb->ID);
			delete pcb;
		}

		/* Exit all threads */
		for (auto tcb : this->Threads)
		{
			if (tcb == nullptr)
			{
				warn("Thread is null? Kernel bug");
				continue;
			}

			debug("Destroying thread \"%s\"(%d)",
				  tcb->Name, tcb->ID);
			delete tcb;
		}

		/* Free Name */
		delete[] this->Name;

		debug("Removing from parent process");
		if (likely(this->Parent))
		{
			this->Parent->Children.erase(std::find(this->Parent->Children.begin(),
												   this->Parent->Children.end(), this));
		}

		debug("Process \"%s\"(%d) destroyed",
			  this->Name, this->ID);
	}
}
