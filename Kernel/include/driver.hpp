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

#ifndef __FENNIX_KERNEL_DRIVER_H__
#define __FENNIX_KERNEL_DRIVER_H__

#include <types.h>

#include <interface/driver.h>
#include <interface/input.h>
#include <interface/block.h>
#include <interface/usb.h>
#include <fs/vfs.hpp>
#include <unordered_map>
#include <memory.hpp>
#include <lock.hpp>
#include <task.hpp>
#include <ring.hpp>
#include <debug.h>
#include <cpu.hpp>
#include <pci.hpp>
#include <irq.hpp>
#include <vector>
#include <bitset>
#include <elf.h>
#include <io.h>
#include <list>

struct BuiltInDriver
{
	char Name[32] = {'\0'};
	char Description[64] = {'\0'};
	char Author[32] = {'\0'};
	struct
	{
		int Major, Minor, Patch;
	} Version = {0, 0, 0};
	char License[32] = {'\0'};
	bool Initialized = false;
	int ErrorCode = 0;

	int (*Entry)() = nullptr;
	int (*Final)() = nullptr;
	int (*Panic)() = nullptr;
	int (*Probe)() = nullptr;
	uintptr_t EntryPoint = 0;
};

extern const BuiltInDriver __kernel_builtin_drivers_start[];
extern const BuiltInDriver __kernel_builtin_drivers_end[];

#define REGISTER_BUILTIN_DRIVER(driverName, desc, auth, maj, min, patch,   \
								entryFunc, finalFunc, panicFunc, initFunc) \
	int __builtin_driver_start_##driverName(dev_t id)                      \
	{                                                                      \
		DriverID = id;                                                     \
		return 0;                                                          \
	}                                                                      \
	static const BuiltInDriver __builtin_driver_##driverName               \
		__attribute__((section(".builtin_drivers"), used)) = {             \
			#driverName,                                                   \
			desc,                                                          \
			auth,                                                          \
			{maj, min, patch},                                             \
			"",                                                            \
			false,                                                         \
			0,                                                             \
			entryFunc,                                                     \
			finalFunc,                                                     \
			panicFunc,                                                     \
			initFunc,                                                      \
			(uintptr_t)__builtin_driver_start_##driverName}

namespace Driver
{
	char GetScanCode(uint8_t ScanCode, bool Upper);
	char GetControlCharacter(KeyScanCodes Key);
	bool IsValidChar(uint8_t ScanCode);

	struct DriverHandlers
	{
		const InodeOperations *Ops = nullptr;
		struct Inode *Node = nullptr;
		RingBuffer<InputReport> *InputReports;
	};

	struct DriverObject : BuiltInDriver
	{
		bool IsBuiltIn = false;
		uintptr_t BaseAddress = 0;
		Memory::VirtualMemoryArea *vma;

		/* Path has the same pointer as in the Node */
		std::string Path;
		std::unordered_map<uint8_t, void *> *InterruptHandlers;
		std::unordered_map<dev_t, DriverHandlers> *DeviceOperations;
		dev_t ID = 0;
	};

	class Manager
	{
	private:
		NewLock(ModuleInitLock);
		std::unordered_map<dev_t, DriverObject> Drivers;

		/**
		 * 0 - generic null/zero/random/etc devices
		 * 1 - input/... devices
		 */
		dev_t DriverIDCounter = 2;
		Node devNode = nullptr;
		Node devInputNode = nullptr;
		Node devBlockNode = nullptr;

		bool IsDriverTrusted(Node File);
		int LoadDriverFile(DriverObject &Drv, Node File);
		void ReloadDriver(dev_t driverID);

		void InitializeDeviceDirectory();

	public:
		RingBuffer<KeyboardReport> GlobalKeyboardInputReports;
		RingBuffer<MouseReport> GlobalMouseInputReports;

		struct DeviceInode
		{
			struct Inode inode;
			Node Parent;
			Inode *ParentInode;
			std::string Name;
			std::vector<DeviceInode *> Children;

			size_t Size;
			time_t AccessTime, ModifyTime, ChangeTime;
			uint32_t BlockSize;
			uint32_t Blocks;
		};

		std::unordered_map<dev_t, DriverObject> &
		GetDrivers() { return Drivers; }

		void Daemon();
		void PreloadDrivers();
		void LoadAllDrivers();
		void UnloadAllDrivers();
		void Panic();

		/** Prefixes:
		 * - dsk (any disk device)
		 *    - dsk0p0 (disk 0, partition 0)
		 * - blk (block device)
		 * - eth (Ethernet device)
		 * - wlan (Wireless LAN device)
		 * - lo (Loopback device)
		 * - kb (Keyboard device)
		 * - ms (Mouse device)
		 * - js (Joystick device)
		 * - tp (Touchpad device)
		 * - tc (Touchscreen device)
		 * - cam (Camera device)
		 * - spk (Speaker device)
		 * - mic (Microphone device)
		 * - snd (Sound device)
		 * - tty (Serial device)
		 * - lp (Parallel device)
		 * - gpu (Graphics device)
		 * - fb (Framebuffer device)
		 * - usb (USB device)
		 *    - usb0dsk0p0 (USB 0, disk 0, partition 0; for USB storage)
		 */
		dev_t CreateIncrementalDevice(dev_t DriverID, const std::string &Prefix, mode_t Mode, InodeOperations *Ops);

		dev_t CreateDeviceFile(dev_t DriverID, const char *name, mode_t mode, const InodeOperations *Operations);

		dev_t RegisterDevice(dev_t DriverID, DeviceType Type, const InodeOperations *Operations);
		int ReportInputEvent(dev_t DriverID, InputReport *Report);
		int UnregisterDevice(dev_t DriverID, dev_t Device);

		dev_t RegisterBlockDevice(dev_t DriverID, struct BlockDevice *Device);
		int UnregisterBlockDevice(dev_t DriverID, dev_t DeviceID);

		void *AllocateMemory(dev_t DriverID, size_t Pages);
		void FreeMemory(dev_t DriverID, void *Pointer, size_t Pages);

		Manager();

	private:
		~Manager();
	};
}

void *GetSymbolByName(const char *Name, int Version);

#ifndef NO_API_IN_HEADER
namespace v0
{
	typedef int CriticalState;

	void KernelPrint(dev_t DriverID, const char *Format, va_list args);
	void KernelLog(dev_t DriverID, const char *Format, va_list args);

	CriticalState EnterCriticalSection(dev_t DriverID);
	void LeaveCriticalSection(dev_t DriverID, CriticalState PreviousState);

	int RegisterInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler);
	int OverrideInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler);
	int UnregisterInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler);
	int UnregisterAllInterruptHandlers(dev_t DriverID, void *Handler);

	dev_t RegisterFileSystem(dev_t DriverID, FileSystemInfo *Info);
	int UnregisterFileSystem(dev_t DriverID, dev_t Device);

	dev_t RegisterBlockDevice(dev_t DriverID, struct BlockDevice *Device);
	int UnregisterBlockDevice(dev_t DriverID, dev_t DeviceID);

	pid_t CreateKernelProcess(dev_t DriverID, const char *Name);
	pid_t CreateKernelThread(dev_t DriverID, pid_t pId, const char *Name, void *EntryPoint, void *Argument);
	pid_t GetCurrentProcess(dev_t DriverID);
	int KillProcess(dev_t DriverID, pid_t pId, int ExitCode);
	int KillThread(dev_t DriverID, pid_t tId, pid_t pId, int ExitCode);
	void Yield(dev_t DriverID);
	void Sleep(dev_t DriverID, uint64_t Milliseconds);

	void *AllocateMemory(dev_t DriverID, size_t Pages);
	void FreeMemory(dev_t DriverID, void *Pointer, size_t Pages);
	void *MemoryCopy(dev_t DriverID, void *Destination, const void *Source, size_t Length);
	void *MemorySet(dev_t DriverID, void *Destination, int Value, size_t Length);
	void *MemoryMove(dev_t DriverID, void *Destination, const void *Source, size_t Length);
	size_t StringLength(dev_t DriverID, const char String[]);
	char *_strstr(dev_t DriverID, const char *Haystack, const char *Needle);

	void MapPages(dev_t MajorID, void *PhysicalAddress, void *VirtualAddress, size_t Pages, uint32_t Flags);
	void UnmapPages(dev_t MajorID, void *VirtualAddress, size_t Pages);
	void AppendMapFlag(dev_t MajorID, void *Address, PageMapFlags Flag);
	void RemoveMapFlag(dev_t MajorID, void *Address, PageMapFlags Flag);

	void *Znwm(size_t Size);
	void ZdlPvm(void *Pointer, size_t Size);

	__PCIArray *GetPCIDevices(dev_t DriverID, uint16_t _Vendors[], uint16_t _Devices[]);
	void InitializePCI(dev_t DriverID, void *_Header);
	uint32_t GetBAR(dev_t DriverID, uint8_t i, void *_Header);
	uint8_t iLine(dev_t DriverID, PCI::PCIDevice *Device);
	uint8_t iPin(dev_t DriverID, PCI::PCIDevice *Device);

	dev_t CreateDeviceFile(dev_t DriverID, const char *name, mode_t mode, const InodeOperations *Operations);
	dev_t RegisterDevice(dev_t DriverID, DeviceType Type, const InodeOperations *Operations);
	int UnregisterDevice(dev_t DriverID, dev_t Device);
	int ReportInputEvent(dev_t DriverID, InputReport *Report);

	USBDevice *CreateUSBDevice(dev_t DriverID);
	int DestroyUSBDevice(dev_t DriverID, USBDevice *Device);
	int InitializeUSBDevice(dev_t DriverID, USBDevice *Device);
	int AddController(dev_t DriverID, USBController *Controller);
	int RemoveController(dev_t DriverID, USBController *Controller);
}
#endif // !NO_API_IN_HEADER

#endif // !__FENNIX_KERNEL_DRIVER_H__
