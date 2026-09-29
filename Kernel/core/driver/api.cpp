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

#define NO_API_IN_HEADER 1
#include <driver.hpp>
#include <interface/driver.h>
#include <interface/fs.h>
#include <type_traits>
#include <interface/input.h>
#include <interface/pci.h>

#include "../../kernel.h"

#define DEBUG_API

#ifdef DEBUG_API
#define dbg_api(Format, ...) func(Format, ##__VA_ARGS__)
#else
#define dbg_api(Format, ...)
#endif

namespace v0
{
	typedef int CriticalState;

	void KernelPrint(dev_t DriverID, const char *Format, va_list args)
	{
		dbg_api("%d, %s, %#lx", DriverID, Format, args);

		/* FIXME! */
		fctprintf(Log::uart_wrapper, nullptr, "DRVER| %ld: ", DriverID);
		vfctprintf(Log::uart_wrapper, nullptr, Format, args);
		Log::uart_wrapper('\n', nullptr);
	}

	void KernelLog(dev_t DriverID, const char *Format, va_list args)
	{
		dbg_api("%d, %s, %#lx", DriverID, Format, args);

		fctprintf(Log::uart_wrapper, nullptr, "DRVER| %ld: ", DriverID);
		vfctprintf(Log::uart_wrapper, nullptr, Format, args);
		Log::uart_wrapper('\n', nullptr);
	}

	/* --------- */

	CriticalState EnterCriticalSection(dev_t DriverID)
	{
		dbg_api("%d", DriverID);

		CriticalState cs;

#if defined(__i386__) || defined(__x86_64__)

		uintptr_t Flags;
#if defined(__x86_64__)
		asmv("pushfq");
		asmv("popq %0" : "=r"(Flags));
#else
		asmv("pushfl");
		asmv("popl %0" : "=r"(Flags));
#endif
		cs = Flags & (1 << 9);
		asmv("cli");

		return cs;

#elif defined(__aarch64__)

		UNUSED(cs);
		stub;

		return 0;

#endif
	}

	void LeaveCriticalSection(dev_t DriverID, CriticalState PreviousState)
	{
		dbg_api("%d, %d", DriverID, PreviousState);

#if defined(__i386__) || defined(__x86_64__)

		if (PreviousState)
			asmv("sti");

#elif defined(__arm__) || defined(__aarch64__)

		if (PreviousState)
			stub;

#endif
	}

	int RegisterInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler)
	{
		dbg_api("%d, %d, %#lx", DriverID, IRQ, Handler);

		std::unordered_map<dev_t, Driver::DriverObject> &drivers =
			DriverManager->GetDrivers();
		const auto it = drivers.find(DriverID);
		if (it == drivers.end())
			ReturnLogError(-EINVAL, "Driver %d not found", DriverID);
		const Driver::DriverObject *drv = &it->second;

		if (drv->InterruptHandlers->contains(IRQ))
			return -EEXIST;

		irq.RegisterHandler((Interrupt::Handle)Handler, IRQ);
		auto ih = drv->InterruptHandlers;
		ih->insert(std::pair<uint8_t, void *>(IRQ, Handler));
		return 0;
	}

	int OverrideInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler)
	{
		dbg_api("%d, %d, %#lx", DriverID, IRQ, Handler);

		debug("Overriding IRQ %d with %#lx", IRQ, Handler);

		std::unordered_map<dev_t, Driver::DriverObject> &drivers =
			DriverManager->GetDrivers();

		for (auto &var : drivers)
		{
			Driver::DriverObject *drv = &var.second;
			for (const auto &ih : *drv->InterruptHandlers)
			{
				if (ih.first != IRQ)
					continue;

				debug("Removing IRQ %d: %#lx for %s", IRQ, (uintptr_t)ih.second, drv->Path.c_str());
				irq.UnregisterHandler((Interrupt::Handle)ih.second, IRQ);
				drv->InterruptHandlers->erase(IRQ);
				break;
			}
		}

		return RegisterInterruptHandler(DriverID, IRQ, Handler);
	}

	int UnregisterInterruptHandler(dev_t DriverID, uint8_t IRQ, void *Handler)
	{
		dbg_api("%d, %d, %#lx", DriverID, IRQ, Handler);

		std::unordered_map<dev_t, Driver::DriverObject> &drivers =
			DriverManager->GetDrivers();
		const auto it = drivers.find(DriverID);
		if (it == drivers.end())
			ReturnLogError(-EINVAL, "Driver %d not found", DriverID);
		const Driver::DriverObject *drv = &it->second;

		irq.UnregisterHandler((Interrupt::Handle)Handler, IRQ);
		auto ih = drv->InterruptHandlers;
		ih->erase(IRQ);
		return 0;
	}

	int UnregisterAllInterruptHandlers(dev_t DriverID, void *Handler)
	{
		dbg_api("%d, %#lx", DriverID, Handler);

		std::unordered_map<dev_t, Driver::DriverObject> &drivers =
			DriverManager->GetDrivers();
		const auto it = drivers.find(DriverID);
		if (it == drivers.end())
			ReturnLogError(-EINVAL, "Driver %d not found", DriverID);
		const Driver::DriverObject *drv = &it->second;

		for (auto &i : *drv->InterruptHandlers)
		{
			irq.UnregisterHandler((Interrupt::Handle)Handler, i.first);
			debug("Removed IRQ %d: %#lx for %s", i.first, (uintptr_t)Handler, drv->Path.c_str());
		}
		auto ih = drv->InterruptHandlers;
		ih->clear();
		return 0;
	}

	/* --------- */

	dev_t RegisterFileSystem(dev_t DriverID, FileSystemInfo *Info)
	{
		dbg_api("%d, %#lx", DriverID, Info);

		return fs->RegisterFileSystem(Info);
	}

	int UnregisterFileSystem(dev_t DriverID, dev_t Device)
	{
		dbg_api("%d, %d", DriverID, Device);

		return fs->UnregisterFileSystem(Device);
	}

	/* --------- */

	pid_t CreateKernelProcess(dev_t DriverID, const char *Name)
	{
		dbg_api("%d, %s", DriverID, Name);

		Tasking::PCB *pcb = TaskManager->CreateProcess(nullptr, Name, Tasking::System,
													   true, 0, 0);

		return pcb->ID;
	}

	pid_t CreateKernelThread(dev_t DriverID, pid_t pId, const char *Name, void *EntryPoint, void *Argument)
	{
		dbg_api("%d, %d, %s, %#lx, %#lx", DriverID, pId, Name, EntryPoint, Argument);

		Tasking::PCB *parent = TaskManager->GetProcessByID(pId);
		if (!parent)
			return -EINVAL;

		CriticalSection cs;
		Tasking::TCB *tcb = TaskManager->CreateThread(parent, (Tasking::IP)EntryPoint);
		if (Argument)
			tcb->SYSV_ABI_Call((uintptr_t)Argument);
		tcb->Rename(Name);
		return tcb->ID;
	}

	pid_t GetCurrentProcess(dev_t DriverID)
	{
		dbg_api("%d", DriverID);

		return TaskManager->GetCurrentProcess()->ID;
	}

	int KillProcess(dev_t DriverID, pid_t pId, int ExitCode)
	{
		dbg_api("%d, %d, %d", DriverID, pId, ExitCode);

		Tasking::PCB *pcb = TaskManager->GetProcessByID(pId);
		if (!pcb)
			return -EINVAL;
		TaskManager->KillProcess(pcb, (Tasking::KillCode)ExitCode);
		return 0;
	}

	int KillThread(dev_t DriverID, pid_t tId, pid_t pId, int ExitCode)
	{
		dbg_api("%d, %d, %d", DriverID, tId, ExitCode);

		Tasking::TCB *tcb = TaskManager->GetThreadByID(tId, TaskManager->GetProcessByID(pId));
		if (!tcb)
			return -EINVAL;
		TaskManager->KillThread(tcb, (Tasking::KillCode)ExitCode);
		return 0;
	}

	void Yield(dev_t DriverID)
	{
		dbg_api("%d", DriverID);

		TaskManager->Yield();
	}

	void Sleep(dev_t DriverID, uint64_t Milliseconds)
	{
		dbg_api("%d, %d", DriverID, Milliseconds);
		thisClock->Sleep(std::chrono::milliseconds(Milliseconds));
	}

	/* --------- */

	void *AllocateMemory(dev_t DriverID, size_t Pages)
	{
		dbg_api("%d, %d", DriverID, Pages);
		return DriverManager->AllocateMemory(DriverID, Pages);
	}

	void FreeMemory(dev_t DriverID, void *Pointer, size_t Pages)
	{
		dbg_api("%d, %#lx, %d", DriverID, Pointer, Pages);
		DriverManager->FreeMemory(DriverID, Pointer, Pages);
	}

	void *MemoryCopy(dev_t DriverID, void *Destination, const void *Source, size_t Length)
	{
		dbg_api("%d, %#lx, %#lx, %d", DriverID, Destination, Source, Length);

		return memcpy(Destination, Source, Length);
	}

	void *MemorySet(dev_t DriverID, void *Destination, int Value, size_t Length)
	{
		dbg_api("%d, %#lx, %d, %d", DriverID, Destination, Value, Length);

		return memset(Destination, Value, Length);
	}

	void *MemoryMove(dev_t DriverID, void *Destination, const void *Source, size_t Length)
	{
		dbg_api("%d, %#lx, %#lx, %d", DriverID, Destination, Source, Length);

		return memmove(Destination, Source, Length);
	}

	size_t StringLength(dev_t DriverID, const char String[])
	{
		dbg_api("%d, %s", DriverID, String);

		return strlen(String);
	}

	char *_strstr(dev_t DriverID, const char *Haystack, const char *Needle)
	{
		dbg_api("%d, %s, %s", DriverID, Haystack, Needle);

		return (char *)strstr(Haystack, Needle);
	}

	void MapPages(dev_t MajorID, void *PhysicalAddress, void *VirtualAddress, size_t Pages, uint32_t Flags)
	{
		dbg_api("%d, %#lx, %#lx, %d, %d", MajorID, PhysicalAddress, VirtualAddress, Pages, Flags);

		Memory::Virtual vmm(KernelPageTable);
		vmm.Map(VirtualAddress, PhysicalAddress, Pages, Flags);
	}

	void UnmapPages(dev_t MajorID, void *VirtualAddress, size_t Pages)
	{
		dbg_api("%d, %#lx, %d", MajorID, VirtualAddress, Pages);

		Memory::Virtual vmm(KernelPageTable);
		vmm.Unmap(VirtualAddress, Pages);
	}

	void AppendMapFlag(dev_t MajorID, void *Address, PageMapFlags Flag)
	{
		dbg_api("%d, %#lx, %d", MajorID, Address, Flag);

		Memory::Virtual vmm(KernelPageTable);
		vmm.GetPTE(Address)->raw |= Flag;
	}

	void RemoveMapFlag(dev_t MajorID, void *Address, PageMapFlags Flag)
	{
		dbg_api("%d, %#lx, %d", MajorID, Address, Flag);

		Memory::Virtual vmm(KernelPageTable);
		vmm.GetPTE(Address)->raw &= ~Flag;
	}

	void *Znwm(size_t Size)
	{
		dbg_api("%d", Size);

		return malloc(Size);
	}

	void ZdlPvm(void *Pointer, size_t Size)
	{
		dbg_api("%d, %#lx", Pointer, Size);

		free(Pointer);
	}

	/* --------- */

	__PCIArray *GetPCIDevices(dev_t DriverID, uint16_t _Vendors[], uint16_t _Devices[])
	{
		dbg_api("%d, %#lx, %#lx", DriverID, _Vendors, _Devices);

		std::unordered_map<dev_t, Driver::DriverObject> &Drivers =
			DriverManager->GetDrivers();

		auto itr = Drivers.find(DriverID);
		if (itr == Drivers.end())
			return nullptr;

		std::list<uint16_t> VendorIDs;
		for (int i = 0; _Vendors[i] != 0x0; i++)
			VendorIDs.push_back(_Vendors[i]);

		std::list<uint16_t> DeviceIDs;
		for (int i = 0; _Devices[i] != 0x0; i++)
			DeviceIDs.push_back(_Devices[i]);

		std::list<PCI::PCIDevice> Devices = PCIManager->FindPCIDevice(VendorIDs, DeviceIDs);
		if (Devices.empty())
			return nullptr;

		Memory::VirtualMemoryArea *vma = itr->second.vma;
		__PCIArray *head = nullptr;
		__PCIArray *array = nullptr;

		for (auto &dev : Devices)
		{
			/* TODO: optimize memory allocation */
			PCI::PCIDevice *dptr = (PCI::PCIDevice *)vma->RequestPages(TO_PAGES(sizeof(PCI::PCIDevice)));
			memcpy(dptr, &dev, sizeof(PCI::PCIDevice));

			__PCIArray *newArray = (__PCIArray *)vma->RequestPages(TO_PAGES(sizeof(__PCIArray)));

			if (unlikely(head == nullptr))
			{
				head = newArray;
				array = head;
			}
			else
			{
				array->Next = newArray;
				array = newArray;
			}

			array->Device = dptr;
			array->Next = nullptr;

			debug("Found %02x.%02x.%02x: %04x:%04x",
				  dev.Bus, dev.Device, dev.Function,
				  dev.Header->VendorID, dev.Header->DeviceID);
		}

		return head;
	}

	void InitializePCI(dev_t DriverID, void *_Header)
	{
		dbg_api("%d, %#lx", DriverID, _Header);
		PCIManager->InitializeDevice(*(PCI::PCIDevice *)_Header, KernelPageTable);
	}

	uint32_t GetBAR(dev_t DriverID, uint8_t i, void *_Header)
	{
		dbg_api("%d, %d, %#lx", DriverID, i, _Header);

		PCI::PCIDevice *__device = (PCI::PCIDevice *)_Header;
		PCI::PCIDeviceHeader *Header = (PCI::PCIDeviceHeader *)__device->Header;

		switch (Header->HeaderType)
		{
		case 128:
			warn("Unknown header type %d! Guessing PCI Header 0",
				 Header->HeaderType);
			[[fallthrough]];
		case 0: /* PCI Header 0 */
		{
			PCI::PCIHeader0 *hdr0 = (PCI::PCIHeader0 *)Header;
			switch (i)
			{
			case 0:
				return hdr0->BAR[0];
			case 1:
				return hdr0->BAR[1];
			case 2:
				return hdr0->BAR[2];
			case 3:
				return hdr0->BAR[3];
			case 4:
				return hdr0->BAR[4];
			case 5:
				return hdr0->BAR[5];
			default:
				assert(!"Invalid BAR index");
			}
		}
		case 1: /* PCI Header 1 (PCI-to-PCI Bridge) */
		{
			PCI::PCIHeader1 *hdr1 = (PCI::PCIHeader1 *)Header;
			switch (i)
			{
			case 0:
				return hdr1->BAR[0];
			case 1:
				return hdr1->BAR[1];
			default:
				assert(!"Invalid BAR index");
			}
		}
		case 2: /* PCI Header 2 (PCI-to-CardBus Bridge) */
		{
			assert(!"PCI-to-CardBus Bridge not supported");
		}
		default:
			assert(!"Invalid PCI header type");
		}
	}

	uint8_t iLine(dev_t DriverID, PCIDevice *Device)
	{
		dbg_api("%d, %#lx", DriverID, Device);

		PCIHeader0 *Header = (PCIHeader0 *)Device->Header;
		return Header->InterruptLine;
	}

	uint8_t iPin(dev_t DriverID, PCIDevice *Device)
	{
		dbg_api("%d, %#lx", DriverID, Device);

		PCIHeader0 *Header = (PCIHeader0 *)Device->Header;
		return Header->InterruptPin;
	}

	/* --------- */

	dev_t CreateDeviceFile(dev_t DriverID, const char *name, mode_t mode, const InodeOperations *Operations)
	{
		dbg_api("%d, %s, %#o, %#lx", DriverID, name, mode, Operations);

		return DriverManager->CreateDeviceFile(DriverID, name, mode, Operations);
	}

	dev_t RegisterDevice(dev_t DriverID, DeviceType Type, const InodeOperations *Operations)
	{
		dbg_api("%d, %d, %#lx", DriverID, Type, Operations);

		return DriverManager->RegisterDevice(DriverID, Type, Operations);
	}

	int UnregisterDevice(dev_t DriverID, dev_t Device)
	{
		dbg_api("%d, %d", DriverID, Device);

		return DriverManager->UnregisterDevice(DriverID, Device);
	}

	int ReportInputEvent(dev_t DriverID, InputReport *Report)
	{
		dbg_api("%d, %#lx", DriverID, Report);

		return DriverManager->ReportInputEvent(DriverID, Report);
	}

	dev_t RegisterBlockDevice(dev_t DriverID, struct BlockDevice *Device)
	{
		dbg_api("%d, %#lx", DriverID, Device);

		return DriverManager->RegisterBlockDevice(DriverID, Device);
	}

	int UnregisterBlockDevice(dev_t DriverID, dev_t DeviceID)
	{
		dbg_api("%d, %d", DriverID, DeviceID);

		return DriverManager->UnregisterBlockDevice(DriverID, DeviceID);
	}

	USBDevice *CreateUSBDevice(dev_t DriverID)
	{
		dbg_api("%d", DriverID);

		return usb->CreateDevice();
	}

	int DestroyUSBDevice(dev_t DriverID, USBDevice *Device)
	{
		dbg_api("%d, %#lx", DriverID, Device);

		return usb->RemoveDevice(Device);
	}

	int InitializeUSBDevice(dev_t DriverID, USBDevice *Device)
	{
		dbg_api("%d, %#lx", DriverID, Device);

		return usb->InitializeDevice(Device);
	}

	int AddController(dev_t DriverID, USBController *Controller)
	{
		dbg_api("%d, %#lx", DriverID, Controller);

		return usb->AddController(Controller);
	}

	int RemoveController(dev_t DriverID, USBController *Controller)
	{
		dbg_api("%d, %#lx", DriverID, Controller);

		return usb->RemoveController(Controller);
	}
}

struct APISymbols
{
	const char *Name;
	void *Function;
};

static struct APISymbols APISymbols_v0[] = {
	{"__KernelPrint", (void *)v0::KernelPrint},
	{"__KernelLog", (void *)v0::KernelLog},

	{"__EnterCriticalSection", (void *)v0::EnterCriticalSection},
	{"__LeaveCriticalSection", (void *)v0::LeaveCriticalSection},

	{"__RegisterInterruptHandler", (void *)v0::RegisterInterruptHandler},
	{"__OverrideInterruptHandler", (void *)v0::OverrideInterruptHandler},
	{"__UnregisterInterruptHandler", (void *)v0::UnregisterInterruptHandler},
	{"__UnregisterAllInterruptHandlers", (void *)v0::UnregisterAllInterruptHandlers},

	{"__RegisterFileSystem", (void *)v0::RegisterFileSystem},
	{"__UnregisterFileSystem", (void *)v0::UnregisterFileSystem},

	{"__CreateKernelProcess", (void *)v0::CreateKernelProcess},
	{"__CreateKernelThread", (void *)v0::CreateKernelThread},
	{"__GetCurrentProcess", (void *)v0::GetCurrentProcess},
	{"__KillProcess", (void *)v0::KillProcess},
	{"__KillThread", (void *)v0::KillThread},
	{"__Yield", (void *)v0::Yield},
	{"__Sleep", (void *)v0::Sleep},

	{"__AllocateMemory", (void *)v0::AllocateMemory},
	{"__FreeMemory", (void *)v0::FreeMemory},
	{"__MemoryCopy", (void *)v0::MemoryCopy},
	{"__MemorySet", (void *)v0::MemorySet},
	{"__MemoryMove", (void *)v0::MemoryMove},
	{"__StringLength", (void *)v0::StringLength},
	{"__strstr", (void *)v0::_strstr},
	{"__MapPages", (void *)v0::MapPages},
	{"__UnmapPages", (void *)v0::UnmapPages},
	{"__AppendMapFlag", (void *)v0::AppendMapFlag},
	{"__RemoveMapFlag", (void *)v0::RemoveMapFlag},
	{"_Znwm", (void *)v0::Znwm},
	{"_ZdlPvm", (void *)v0::ZdlPvm},

	{"__GetPCIDevices", (void *)v0::GetPCIDevices},
	{"__InitializePCI", (void *)v0::InitializePCI},
	{"__GetBAR", (void *)v0::GetBAR},
	{"__iLine", (void *)v0::iLine},
	{"__iPin", (void *)v0::iPin},

	{"__CreateDeviceFile", (void *)v0::CreateDeviceFile},
	{"__RegisterDevice", (void *)v0::RegisterDevice},
	{"__UnregisterDevice", (void *)v0::UnregisterDevice},
	{"__ReportInputEvent", (void *)v0::ReportInputEvent},
	{"__RegisterBlockDevice", (void *)v0::RegisterBlockDevice},
	{"__UnregisterBlockDevice", (void *)v0::UnregisterBlockDevice},

	{"__CreateUSBDevice", (void *)v0::CreateUSBDevice},
	{"__DestroyUSBDevice", (void *)v0::DestroyUSBDevice},
	{"__InitializeUSB", (void *)v0::InitializeUSBDevice},
	{"__AddController", (void *)v0::AddController},
	{"__RemoveController", (void *)v0::RemoveController},
};

long __KernelUndefinedFunction(long arg0, long arg1, long arg2, long arg3,
							   long arg4, long arg5, long arg6, long arg7)
{
	debug("%#lx, %#lx, %#lx, %#lx, %#lx, %#lx, %#lx, %#lx",
		  arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	assert(!"Undefined kernel driver API function called!");
	CPU::Stop();
}

void *GetSymbolByName(const char *Name, int Version)
{
	switch (Version)
	{
	case 0:
	{
		for (auto sym : APISymbols_v0)
		{
			if (strcmp(Name, sym.Name) != 0)
				continue;

			debug("Symbol %s found in API version %d", Name, Version);
			return sym.Function;
		}
		break;
	}
	default:
		assert(!"Invalid API version");
	}

	error("Symbol %s not found in API version %d", Name, Version);
	klog("Driver API symbol \"%s\" not found!", Name);
	return (void *)__KernelUndefinedFunction;
}
