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

#include "kernel.h"
#ifdef DEBUG
#include "tests/t.h"
#endif

#include <fs/ustar.hpp>
#include <subsystems.hpp>
#include <kshell.hpp>
#include <power.hpp>
#include <lock.hpp>
#include <printf.h>
#include <exec.hpp>
#include <kcon.hpp>
#include <cwalk.h>
#include <vm.hpp>
#include <vector>

int SpawnNativeInit()
{
	const char *envp[] = {
		"PATH=/bin:/usr/bin",
		"LD_LIBRARY_PATH=/lib:/usr/lib",
		"TERM=tty",
		"HOME=/home/root",
		"USER=root",
		"TZ=UTC",
		nullptr};

	const char *argv[] = {Config.InitPath, nullptr};

	return Execute::Spawn(Config.InitPath, argv, envp, nullptr, false, Tasking::Native, true);
}

int SpawnLinuxInit()
{
	const char *envp[] = {
		"PATH=/bin:/usr/bin",
		"LD_LIBRARY_PATH=/lib:/usr/lib",
		"TERM=tty",
		"HOME=/root",
		"USER=root",
		"TZ=UTC",
		nullptr};

	std::string init = Config.InitPath;
	std::vector<std::string> fallbackPaths = {
		init,
		"/bin/init",
		"/sbin/init",
		"/system/init",
		"/usr/bin/init",
		"/boot/init",
		"/startup/init"};

	const char *foundPath = nullptr;
	Node root = fs->GetRoot(1);
	for (const std::string &path : fallbackPaths)
	{
		const char *str = path.c_str();
		if (fs->Lookup(root, str) == false)
			continue;
		foundPath = str;
		break;
	}

	if (!foundPath)
	{
		error("No valid init found in fallback paths");
		return -ENOENT;
	}

	const char *argv[] = {foundPath, nullptr};
	return Execute::Spawn(foundPath, argv, envp, nullptr, false, Tasking::Linux, true);
}

int SpawnInit()
{
	if (Config.LinuxSubsystem)
		return SpawnLinuxInit();
	else
		return SpawnNativeInit();
}

void main_thread()
{
	Log::LateInit();
	thisThread->SetPriority(Tasking::Critical);

#ifdef DEBUG
	StressKernel();
	// TaskManager->CreateThread(thisProcess, Tasking::IP(tasking_test_fb));
	// TaskManager->CreateThread(thisProcess, Tasking::IP(tasking_test_mutex));
	// ilp;
	TaskManager->CreateThread(thisProcess, Tasking::IP(TaskMgr));
	TaskManager->CreateThread(thisProcess, Tasking::IP(TaskHeartbeat));
	TreeFS(fs->GetRoot(0), 0);
	coroutineTest();
#endif

	klog("Kernel compiled using GCC %d.%d.%d as of %s %s with Standard C++ %dL",
		 __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,
		 __DATE__, __TIME__,
		 __cplusplus);

	if (IsVirtualizedEnvironment())
		klog("Running in a virtualized environment");

	klog("Initializing USB Subsystem");
	usb = new UniversalSerialBus::Manager;

	klog("Initializing Driver Manager");
	DriverManager = new Driver::Manager;

	klog("Loading Drivers");
	DriverManager->PreloadDrivers();
	DriverManager->LoadAllDrivers();

	KernelConsole::LateInit();
#ifdef DEBUG
	// TaskManager->CreateThread(thisProcess,
	// 						  Tasking::IP(KShellThread))
	// 	->Rename("Kernel Shell");
#endif

	klog("Loading Subsystems");
	Subsystem::Linux::InitializeSubSystem();
	Subsystem::Windows::InitializeSubSystem();

#ifdef DEBUG
	__late_playground();
#endif

	klog("Executing %s", Config.InitPath);
	int ExitCode = -1;
	Tasking::PCB *initProc;
	Tasking::TCB *initThread;
	int tid = SpawnInit();
	if (tid < 0)
	{
		klog("\x1b[1;37;41mFailed to start init program! Error: %s (%d)\x1b[0m", strerror(tid), tid);
		goto Exit;
	}

	klog("Waiting for init program to start...");
	thisThread->SetPriority(Tasking::Idle);

	initProc = TaskManager->GetProcessByID(tid);
	initThread = TaskManager->GetThreadByID(tid, initProc);
	TaskManager->WaitForThread(initThread);
	if (initThread)
		ExitCode = initThread->GetExitCode();
	else
		ExitCode = 0xEBAD;
Exit:
	klog("\x1b[31mUserspace process exited with code %d (%#x)\x1b[0m",
		 ExitCode, ExitCode < 0 ? -ExitCode : ExitCode);

	klog("Dropping to kernel shell");
	TaskManager->Sleep((1000 * 1'000'000ULL));
	TaskManager->CreateThread(thisProcess, Tasking::IP(KShellThread))->Rename("Kernel Shell");
	CPU::Halt(true);
}
