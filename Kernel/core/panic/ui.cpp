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

#if defined(__amd64__)
#include "../../arch/x86/amd64/cpu/gdt.hpp"
#elif defined(__i386__)
#include "../../arch/i386/cpu/gdt.hpp"
#include "../arch/i386/cpu/apic.hpp"
#elif defined(__aarch64__)
#endif

#include "../../kernel.h"

extern void ExPrint(const char *Format, ...);
extern void DiagnosticDataCollection();
extern void InitFont();
extern KernelConsole::FontRenderer CrashFontRenderer;

extern void *FbBeforePanic;

enum Keys
{
	KEY_INVALID = 0x0,
	KEY_D_ESCAPE = 0x1,
	KEY_D_1 = 0x2,
	KEY_D_2 = 0x3,
	KEY_D_3 = 0x4,
	KEY_D_4 = 0x5,
	KEY_D_5 = 0x6,
	KEY_D_6 = 0x7,
	KEY_D_7 = 0x8,
	KEY_D_8 = 0x9,
	KEY_D_9 = 0xa,
	KEY_D_0 = 0xb,
	KEY_D_MINUS = 0xc,
	KEY_D_EQUALS = 0xd,
	KEY_D_BACKSPACE = 0xe,
	KEY_D_TAB = 0xf,
	KEY_D_Q = 0x10,
	KEY_D_W = 0x11,
	KEY_D_E = 0x12,
	KEY_D_R = 0x13,
	KEY_D_T = 0x14,
	KEY_D_Y = 0x15,
	KEY_D_U = 0x16,
	KEY_D_I = 0x17,
	KEY_D_O = 0x18,
	KEY_D_P = 0x19,
	KEY_D_LBRACKET = 0x1a,
	KEY_D_RBRACKET = 0x1b,
	KEY_D_RETURN = 0x1c,
	KEY_D_LCTRL = 0x1d,
	KEY_D_A = 0x1e,
	KEY_D_S = 0x1f,
	KEY_D_D = 0x20,
	KEY_D_F = 0x21,
	KEY_D_G = 0x22,
	KEY_D_H = 0x23,
	KEY_D_J = 0x24,
	KEY_D_K = 0x25,
	KEY_D_L = 0x26,
	KEY_D_SEMICOLON = 0x27,
	KEY_D_APOSTROPHE = 0x28,
	KEY_D_GRAVE = 0x29,
	KEY_D_LSHIFT = 0x2a,
	KEY_D_BACKSLASH = 0x2b,
	KEY_D_Z = 0x2c,
	KEY_D_X = 0x2d,
	KEY_D_C = 0x2e,
	KEY_D_V = 0x2f,
	KEY_D_B = 0x30,
	KEY_D_N = 0x31,
	KEY_D_M = 0x32,
	KEY_D_COMMA = 0x33,
	KEY_D_PERIOD = 0x34,
	KEY_D_SLASH = 0x35,
	KEY_D_RSHIFT = 0x36,
	KEY_D_PRTSC = 0x37,
	KEY_D_LALT = 0x38,
	KEY_D_SPACE = 0x39,
	KEY_D_CAPSLOCK = 0x3a,
	KEY_D_NUMLOCK = 0x45,
	KEY_D_SCROLLLOCK = 0x46,

	KEY_D_KP_MULTIPLY = 0x37,
	KEY_D_KP_7 = 0x47,
	KEY_D_KP_8 = 0x48,
	KEY_D_KP_9 = 0x49,
	KEY_D_KP_MINUS = 0x4a,
	KEY_D_KP_4 = 0x4b,
	KEY_D_KP_5 = 0x4c,
	KEY_D_KP_6 = 0x4d,
	KEY_D_KP_PLUS = 0x4e,
	KEY_D_KP_1 = 0x4f,
	KEY_D_KP_2 = 0x50,
	KEY_D_KP_3 = 0x51,
	KEY_D_KP_0 = 0x52,
	KEY_D_KP_PERIOD = 0x53,

	KEY_D_F1 = 0x3b,
	KEY_D_F2 = 0x3c,
	KEY_D_F3 = 0x3d,
	KEY_D_F4 = 0x3e,
	KEY_D_F5 = 0x3f,
	KEY_D_F6 = 0x40,
	KEY_D_F7 = 0x41,
	KEY_D_F8 = 0x42,
	KEY_D_F9 = 0x43,
	KEY_D_F10 = 0x44,
	KEY_D_F11 = 0x57,
	KEY_D_F12 = 0x58,

	KEY_D_UP = 0x48,
	KEY_D_LEFT = 0x4b,
	KEY_D_RIGHT = 0x4d,
	KEY_D_DOWN = 0x50,

	KEY_U_ESCAPE = 0x81,
	KEY_U_1 = 0x82,
	KEY_U_2 = 0x83,
	KEY_U_3 = 0x84,
	KEY_U_4 = 0x85,
	KEY_U_5 = 0x86,
	KEY_U_6 = 0x87,
	KEY_U_7 = 0x88,
	KEY_U_8 = 0x89,
	KEY_U_9 = 0x8a,
	KEY_U_0 = 0x8b,
	KEY_U_MINUS = 0x8c,
	KEY_U_EQUALS = 0x8d,
	KEY_U_BACKSPACE = 0x8e,
	KEY_U_TAB = 0x8f,
	KEY_U_Q = 0x90,
	KEY_U_W = 0x91,
	KEY_U_E = 0x92,
	KEY_U_R = 0x93,
	KEY_U_T = 0x94,
	KEY_U_Y = 0x95,
	KEY_U_U = 0x96,
	KEY_U_I = 0x97,
	KEY_U_O = 0x98,
	KEY_U_P = 0x99,
	KEY_U_LBRACKET = 0x9a,
	KEY_U_RBRACKET = 0x9b,
	KEY_U_RETURN = 0x9c,
	KEY_U_LCTRL = 0x9d,
	KEY_U_A = 0x9e,
	KEY_U_S = 0x9f,
	KEY_U_D = 0xa0,
	KEY_U_F = 0xa1,
	KEY_U_G = 0xa2,
	KEY_U_H = 0xa3,
	KEY_U_J = 0xa4,
	KEY_U_K = 0xa5,
	KEY_U_L = 0xa6,
	KEY_U_SEMICOLON = 0xa7,
	KEY_U_APOSTROPHE = 0xa8,
	KEY_U_GRAVE = 0xa9,
	KEY_U_LSHIFT = 0xaa,
	KEY_U_BACKSLASH = 0xab,
	KEY_U_Z = 0xac,
	KEY_U_X = 0xad,
	KEY_U_C = 0xae,
	KEY_U_V = 0xaf,
	KEY_U_B = 0xb0,
	KEY_U_N = 0xb1,
	KEY_U_M = 0xb2,
	KEY_U_COMMA = 0xb3,
	KEY_U_PERIOD = 0xb4,
	KEY_U_SLASH = 0xb5,
	KEY_U_RSHIFT = 0xb6,
	KEY_U_KP_MULTIPLY = 0xb7,
	KEY_U_LALT = 0xb8,
	KEY_U_SPACE = 0xb9,
	KEY_U_CAPSLOCK = 0xba,
	KEY_U_F1 = 0xbb,
	KEY_U_F2 = 0xbc,
	KEY_U_F3 = 0xbd,
	KEY_U_F4 = 0xbe,
	KEY_U_F5 = 0xbf,
	KEY_U_F6 = 0xc0,
	KEY_U_F7 = 0xc1,
	KEY_U_F8 = 0xc2,
	KEY_U_F9 = 0xc3,
	KEY_U_F10 = 0xc4,
	KEY_U_NUMLOCK = 0xc5,
	KEY_U_SCROLLLOCK = 0xc6,
	KEY_U_KP_7 = 0xc7,
	KEY_U_KP_8 = 0xc8,
	KEY_U_KP_9 = 0xc9,
	KEY_U_KP_MINUS = 0xca,
	KEY_U_KP_4 = 0xcb,
	KEY_U_KP_5 = 0xcc,
	KEY_U_KP_6 = 0xcd,
	KEY_U_KP_PLUS = 0xce,
	KEY_U_KP_1 = 0xcf,
	KEY_U_KP_2 = 0xd0,
	KEY_U_KP_3 = 0xd1,
	KEY_U_KP_0 = 0xd2,
	KEY_U_KP_PERIOD = 0xd3,
	KEY_U_F11 = 0xd7,
	KEY_U_F12 = 0xd8,
};

struct StackFrame
{
	struct StackFrame *bp;
	uintptr_t ip;
};

struct x86ExceptionName
{
	const char *Mnemonic;
	const char *Name;
	const char *Cause;
};

/* AMD64 Programmer's Manual Volume 2: 8.2 Vectors */
x86ExceptionName x86Exceptions[] = {
	/* 0*/ {"#DE", "Divide-by-Zero-Error", "DIV, IDIV, AAM instructions"},
	/* 1*/ {"#DB", "Debug", "Instruction accesses and data accesses"},
	/* 2*/ {"NMI", "Non-Maskable-Interrupt", "Non-maskable interrupt"},
	/* 3*/ {"#BP", "Breakpoint", "INT3 instruction"},
	/* 4*/ {"#OF", "Overflow", "INTO instruction"},
	/* 5*/ {"#BR", "Bound-Range", "BOUND instruction"},
	/* 6*/ {"#UD", "Invalid-Opcode", "Invalid instructions"},
	/* 7*/ {"#NM", "Device-Not-Available", "x87 instructions"},
	/* 8*/ {"#DF", "Double-Fault", "Exception during the handling of another exception or interrupt"},
	/* 9*/ {"#--", "Coprocessor-Segment-Overrun", "Unsupported (Reserved)"},
	/*10*/ {"#TS", "Invalid-TSS", "Task-state segment access and task switch"},
	/*11*/ {"#NP", "Segment-Not-Present", "Segment register loads"},
	/*12*/ {"#SS", "Stack", "SS register loads and stack references"},
	/*13*/ {"#GP", "General-Protection", "Memory accesses and protection checks"},
	/*14*/ {"#PF", "Page-Fault", "Memory accesses when paging enabled"},
	/*15*/ {"#r0", "Reserved", "Reserved"},
	/*16*/ {"#MF", "x87 Floating-Point Exception-Pending", "x87 floating-point instructions"},
	/*17*/ {"#AC", "Alignment-Check", "Misaligned memory accesses"},
	/*18*/ {"#MC", "Machine-Check", "Model specific"},
	/*19*/ {"#XF", "SIMD Floating-Point", "SSE floating-point instructions"},
	/*20*/ {"#VE", "Virtualization Exception", "Virtualization event"}, /* AMD says this is reserved */
	/*21*/ {"#CP", "Control-Protection Exception", "RET/IRET or other control transfer"},
	/*22*/ {"#r1", "Reserved", "Reserved"},
	/*23*/ {"#r2", "Reserved", "Reserved"},
	/*24*/ {"#r3", "Reserved", "Reserved"},
	/*25*/ {"#r4", "Reserved", "Reserved"},
	/*26*/ {"#r5", "Reserved", "Reserved"},
	/*27*/ {"#r6", "Reserved", "Reserved"},
	/*28*/ {"#HV", "Hypervisor Injection Exception", "Event injection"},
	/*29*/ {"#VC", "VMM Communication Exception", "Virtualization event"},
	/*30*/ {"#SX", "Security Exception", "Security-sensitive event in host"},
	/*31*/ {"#r7", "Reserved", "Reserved"},
};

int ActiveScreen = 0;

char __modSym[20];
nsa const char *ExGetKSymbolByAddress(uintptr_t Address)
{
	if (Address < (uintptr_t)&_kernel_start &&
		Address > (uintptr_t)&_kernel_end)
		return "<OUTSIDE KERNEL>";

	if (!KernelSymbolTable)
		return "<UNKNOWN>";

	const char *sym = KernelSymbolTable->GetSymbol(Address);

	size_t symLen = strlen(sym);

	if (symLen > 16)
	{
		strncpy(__modSym, sym, 16);
		__modSym[16] = '.';
		__modSym[17] = '.';
		__modSym[18] = '.';
		__modSym[19] = '\0';
		sym = __modSym;
	}

	if (unlikely(symLen > 128))
		warn("Symbol \"%s\" is too long! Memory corrupted?", sym);
	return sym;
}

nsa const char *ExGetKSymbol(CPU::ExceptionFrame *Frame)
{
#if defined(__amd64__)
	if (Frame->rip < (uintptr_t)&_kernel_start &&
		Frame->rip > (uintptr_t)&_kernel_end)
		return "<OUTSIDE KERNEL>";
#elif defined(__i386__)
	if (Frame->eip < (uintptr_t)&_kernel_start &&
		Frame->eip > (uintptr_t)&_kernel_end)
		return "<OUTSIDE KERNEL>";
#elif defined(__aarch64__)
#warning "aarch64 not implemented"
#endif

#if defined(__amd64__)
	return ExGetKSymbolByAddress(Frame->rip);
#elif defined(__i386__)
	return ExGetKSymbolByAddress(Frame->eip);
#elif defined(__aarch64__)
#warning "aarch64 not implemented"
	return "stub";
#endif
}

nsa char *TrimWhiteSpace(char *str)
{
	char *end;
	while (*str == ' ')
		str++;
	if (*str == 0)
		return str;
	end = str + strlen(str) - 1;
	while (end > str && *end == ' ')
		end--;
	*(end + 1) = 0;
	return str;
}

nsa void ExDumpData(void *Address, unsigned long Length)
{
	ExPrint("-------------------------------------------------------------------------\n");
	unsigned char *AddressChar = (unsigned char *)Address;
	unsigned char Buffer[17];
	unsigned long Iterate;
	for (Iterate = 0; Iterate < Length; Iterate++)
	{
		if ((Iterate % 16) == 0)
		{
			if (Iterate != 0)
				ExPrint("  %s\n", Buffer);
			ExPrint("  %04x ", Iterate);
		}
		ExPrint(" %02x", AddressChar[Iterate]);
		if ((AddressChar[Iterate] < 0x20) || (AddressChar[Iterate] > 0x7E))
			Buffer[Iterate % 16] = '.';
		else
			Buffer[Iterate % 16] = AddressChar[Iterate];
		Buffer[(Iterate % 16) + 1] = '\0';
	}

	while ((Iterate % 16) != 0)
	{
		ExPrint("   ");
		Iterate++;
	}

	ExPrint("  %s\n", Buffer);
	ExPrint("-------------------------------------------------------------------------\n\n.");
}

nsa void DisplayTopOverlay()
{
	ExPrint("\x1b[H%s %s %s %s \x1b[0m| ",
			ActiveScreen == 0 ? "\x1b[1;37m"
								"MAIN   "
							  : "\x1b[0m"
								"M",
			ActiveScreen == 1 ? "\x1b[1;37m"
								"DETAIL "
							  : "\x1b[0m"
								"D",
			ActiveScreen == 2 ? "\x1b[1;37m"
								"STACK  "
							  : "\x1b[0m"
								"S",
			ActiveScreen == 3 ? "\x1b[1;37m"
								"PROCESS"
							  : "\x1b[0m"
								"P");

	ExPrint("%s %s %s | ", KERNEL_NAME, KERNEL_ARCH, KERNEL_VERSION);
	ExPrint("%ld | ", TO_MiB(KernelAllocator.GetFreeMemory()));
	ExPrint("%s %s", CPU::Hypervisor(), CPU::Vendor());

	ExPrint("\x1b[3;1H");

	/* https://imgflip.com/i/77slbl */
	if ((Random::rand32() % 100) >= 98)
	{
		debug("Easter egg activated!");
		int BaseXOffset = Display->GetWidth - 14;
		int BaseYOffset = 8;
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 0, 0x21852E);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 0, 0x21852E);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 0, 0x21852E);
		Display->SetPixel(BaseXOffset + 7, BaseYOffset + 0, 0x21852E);

		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 7, BaseYOffset + 1, 0x21852E);
		Display->SetPixel(BaseXOffset + 8, BaseYOffset + 1, 0x21852E);

		Display->SetPixel(BaseXOffset + 1, BaseYOffset + 2, 0x21852E);
		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 2, 0x21852E);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 2, 0xFFFFFF);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 2, 0x000000);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 2, 0x21852E);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 2, 0xFFFFFF);
		Display->SetPixel(BaseXOffset + 7, BaseYOffset + 2, 0x000000);
		Display->SetPixel(BaseXOffset + 8, BaseYOffset + 2, 0x21852E);

		Display->SetPixel(BaseXOffset + 1, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 3, 0x21852E);
		Display->SetPixel(BaseXOffset + 7, BaseYOffset + 3, 0x21852E);

		Display->SetPixel(BaseXOffset + 0, BaseYOffset + 4, 0x21852E);
		Display->SetPixel(BaseXOffset + 1, BaseYOffset + 4, 0x21852E);
		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 4, 0x21852E);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 4, 0x21852E);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 4, 0xA84832);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 4, 0xA84832);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 4, 0xA84832);
		Display->SetPixel(BaseXOffset + 7, BaseYOffset + 4, 0xA84832);

		Display->SetPixel(BaseXOffset + 0, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 1, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 5, 0x21852E);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 5, 0x21852E);

		Display->SetPixel(BaseXOffset + 0, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 1, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 2, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 3, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 4, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 5, BaseYOffset + 6, 0x1216FF);
		Display->SetPixel(BaseXOffset + 6, BaseYOffset + 6, 0x1216FF);

		Display->UpdateBuffer();
	}
}

nsa void DisplayBottomOverlay()
{
	ExPrint("\x1b[%d;%dH> \x1b[1;37m", (Display->GetWidth / CrashFontRenderer.CurrentFont->GetInfo().Width) - 1, 1);
}

nsa void DisplayMainScreen(CPU::ExceptionFrame *Frame)
{
	ExPrint("\nWe're sorry, but the system has encountered a critical error and needs to restart.\n");

	ExPrint("\nError: %s (%s 0x%x)\n",
#if defined(__amd64__) || defined(__i386__)
			x86Exceptions[Frame->InterruptNumber].Name,
			x86Exceptions[Frame->InterruptNumber].Mnemonic,
#elif defined(__aarch64__)
			"stub",
			"stub",
#warning "aarch64 not implemented"
#elif defined(__arm__)
			"stub",
			"stub",
#warning "arm not implemented"
#endif
#if defined(__amd64__) || defined(__i386__)
			Frame->InterruptNumber);
#elif defined(__aarch64__)
			0);
#warning "aarch64 not implemented"
#elif defined(__arm__)
			0);
#warning "arm not implemented"
#endif
#if defined(__amd64__) || defined(__i386__)
	ExPrint("Cause: %s\n", x86Exceptions[Frame->InterruptNumber].Cause);
#endif
	ExPrint("Exception occurred in function %s (%#lx)\n",
			ExGetKSymbol(Frame),
#if defined(__amd64__)
			Frame->rip);
#elif defined(__i386__)
			Frame->eip);
#elif defined(__arm__)
			0);
#elif defined(__aarch64__)
0);
#warning "aarch64 not implemented"
#endif

	CPUData *core = GetCurrentCPU();
	if (TaskManager)
	{
		ExPrint("Core: %d / pid: %d / tid: %d\n", core->ID,
				core->CurrentProcess->ID, core->CurrentThread->ID);
	}
	else if (core->IsActive)
		ExPrint("Core: %d\n", core->ID);

	ExPrint("\nWhat to do:\n");
	ExPrint("  1. Restart your device and see if the problem persists.\n");
	ExPrint("  2. Remove any newly installed hardware.\n");
	ExPrint("  3. Check for any available updates for your operating system.\n");
	ExPrint("  4. Uninstall any recently installed drivers or software.\n");
	ExPrint("  If none of the above steps resolve the issue, create a new issue on https://github.com/Fennix-Project/Fennix for further assistance.\n");

	ExPrint("\nUse command 'diag' to create a diagnostic report.\n");
}

arch void DisplayDetailsScreen(CPU::ExceptionFrame *Frame);

nsa void DisplayStackScreen(CPU::ExceptionFrame *Frame)
{
	Memory::Virtual vmm;
	struct StackFrame *sf = nullptr;
#if defined(__amd64__)
	sf = (struct StackFrame *)Frame->rbp;
#elif defined(__i386__)
	sf = (struct StackFrame *)Frame->ebp;
#endif

	ExPrint("\nStack trace (%#lx):\n", sf);

	if (!vmm.Check(sf))
	{
#if defined(__amd64__) || defined(__i386__)
		void *ptr = ((Memory::PageTable *)Frame->cr3)->Get(sf);
		debug("Virtual pointer %#lx -> %#lx", sf, ptr);
		if (vmm.Check(ptr))
			sf = (struct StackFrame *)ptr;
		else
		{
			ExPrint("\x1b[31m< No stack trace available. >\x1b[0m\n");
			return;
		}
#endif
	}

	/* FIXME: Get symbol offset more efficiently */

	uintptr_t fIP = 0;
#if defined(__amd64__)
	fIP = Frame->rip;
#elif defined(__i386__)
	fIP = Frame->eip;
#elif defined(__aarch64__)
#warning "aarch64 not implemented"
#endif

	ExPrint("%p", (void *)fIP);
	ExPrint(" in ");
	if ((fIP >= (uintptr_t)&_kernel_start &&
		 fIP <= (uintptr_t)&_kernel_end))
	{
		const char *sym = KernelSymbolTable->GetSymbol(fIP);
		ssize_t offset = fIP - KernelSymbolTable->GetSymbol(sym);
		if (offset < 0)
			offset = -offset;

		ExPrint("%s+%#lx \x1b[31m<- Exception\x1b[0m\n",
				sym, offset);
	}
	else
		ExPrint("??? \x1b[31m<- Exception\x1b[0m\n");

	if (!sf || !sf->ip || !sf->bp)
	{
		ExPrint("\n\x1b[31m< No stack trace available. >\x1b[0m\n");
		return;
	}

	for (int i = 0; i < 16; ++i)
	{
		if (!sf->ip)
			break;

		ExPrint("%p", (void *)sf->ip);
		ExPrint(" in ");
		if ((sf->ip >= (uintptr_t)&_kernel_start &&
			 sf->ip <= (uintptr_t)&_kernel_end))
		{
			const char *sym = KernelSymbolTable->GetSymbol(sf->ip);
			ssize_t offset = sf->ip - KernelSymbolTable->GetSymbol(sym);
			if (offset < 0)
				offset = -offset;

			ExPrint("%s+%#lx\n", sym, offset);
		}
		else
			ExPrint("???\n");

		if (!vmm.Check(sf->bp))
			return;
		sf = sf->bp;
	}
}

nsa void DisplayProcessScreen(CPU::ExceptionFrame *Frame, bool IgnoreReady = true)
{
	const char *StatusColor[] = {
		"31m",	 // Unknown
		"1;33m", // Ready
		"32m",	 // Running
		"1;33m", // Sleeping
		"1;33m", // Blocked
		"1;33m", // Stopped
		"1;33m", // Waiting

		"35m",	 // Core dump
		"1;31m", // Zombie
		"31m",	 // Terminated
		"1;33m", // Frozen
	};

	const char *StatusString[] = {
		"UNK", // Unknown
		"RDY", // Ready
		"RUN", // Running
		"SLP", // Sleeping
		"BLK", // Blocked
		"STP", // Stopped
		"WTG", // Waiting

		"CRD", // Core dump
		"ZMB", // Zombie
		"TRM", // Terminated
		"FRZ", // Frozen
	};

	if (!TaskManager)
	{
		ExPrint("Tasking is not initialized\n");
		return;
	}

	size_t textLimit = 32;
	if (Display->GetWidth > 800 && Display->GetHeight > 600)
		textLimit = 128;

	std::vector<Tasking::PCB *> Plist = TaskManager->GetProcessList();

	ExPrint("\nProcess list (%ld):\n", Plist.size());
	bool pRdy = false;
	bool showNote = false;
	/* FIXME: This is slow */
	for (auto Process : Plist)
	{
		bool ignore = true;
		if (Process->State == Tasking::Ready && IgnoreReady)
		{
			for (auto Thread : Process->Threads)
			{
				if (Thread->State == Tasking::Ready)
					continue;
				ignore = false;
				break;
			}

			if (ignore)
			{
				pRdy = true;
				debug("Ignoring ready process %s(%d)",
					  Process->Name, Process->ID);
				showNote = true;
				continue;
			}
		}

		ExPrint("-> %.*s%s(%ld) \x1b[%s%s\x1b[0m [exe: %s]\n",
				textLimit, Process->Name,
				strlen(Process->Name) > textLimit ? "..." : "",
				Process->ID, StatusColor[Process->State.load()],
				StatusString[Process->State.load()],
				Process->Executable
					? Process->Executable->Name.c_str()
					: "none");

		bool tRdy = false;
		for (auto Thread : Process->Threads)
		{
			if (Thread->State == Tasking::Ready && IgnoreReady)
			{
				tRdy = true;
				debug("Ignoring ready thread %s(%d)",
					  Thread->Name, Thread->ID);
				showNote = true;
				continue;
			}

			ExPrint("  -> %.*s%s(%ld) \x1b[%s%s\x1b[0m\n",
					textLimit, Thread->Name,
					strlen(Thread->Name) > textLimit ? "..." : "",
					Thread->ID, StatusColor[Thread->State.load()],
					StatusString[Thread->State.load()]);
		}
		if (tRdy)
			ExPrint("  -> ...\n");
	}
	if (pRdy)
		ExPrint("-> ...\n");

	if (showNote)
		ExPrint("Note: Some processes may not be displayed.\n");
}

void UserInput(char *Input);
void ArrowInput(uint8_t key);
CPU::ExceptionFrame *ExFrame;
nsa void DisplayCrashScreen(CPU::ExceptionFrame *Frame)
{
	ExFrame = Frame;

	/* TODO */
	// void *BufferBeforeUpdate = KernelAllocator.RequestPages(TO_PAGES(Display->GetSize));
	// if (BufferBeforeUpdate)
	// 	memcpy(BufferBeforeUpdate, Display->GetBuffer, Display->GetSize);

	Display->ClearBuffer();
	ExPrint("\x1b[2J\x1b[H");
	if (Config.InterruptsOnCrash == false)
	{
		DisplayMainScreen(Frame);
		CPU::Stop();
	}

	DisplayTopOverlay();
	DisplayMainScreen(Frame);
	DisplayStackScreen(Frame);
	DisplayBottomOverlay();

#ifdef DEBUG
#if defined(__amd64__) || defined(__i386__)
	static int once = 0;
	static uint8_t com4 = 0xFF;
	if (!once++)
		com4 = inb(0x2E8);
	if (com4 == 0xFF)
		CPU::Halt(true);

	char UserInputBuffer[256]{'\0'};
	int BackSpaceLimit = 0;

	while (true)
	{
		while ((inb(0x2E8 + 5) & 1) == 0)
			CPU::Pause();
		char key = inb(0x2E8);
		// debug("key: %d", key);

		if (key == '\x7f') /* Backspace (DEL) */
		{
			if (BackSpaceLimit <= 0)
				continue;

			char keyBuf[5] = {'\b', '\x1b', '[', 'K', '\0'};
			ExPrint(keyBuf);
			backspace(UserInputBuffer);
			BackSpaceLimit--;
			continue;
		}
		else if (key == '\x0d') /* Enter (CR) */
		{
			UserInput(UserInputBuffer);
			BackSpaceLimit = 0;
			UserInputBuffer[0] = '\0';
			continue;
		}
		else if (key == '\x1b') /* Escape */
		{
			char tmp[16]{'\0'};
			append(tmp, key);

			while ((inb(0x2E8 + 5) & 1) == 0)
				CPU::Pause();
			char key = inb(0x2E8);
			append(tmp, key);

			if (key == '[')
			{
				// 27 91
				// <  68
				// > 67
				// 	down 66
				// 	up 65
				while ((inb(0x2E8 + 5) & 1) == 0)
					CPU::Pause();
				key = inb(0x2E8);
				append(tmp, key);
				switch (key)
				{
				case 'A':
					key = KEY_D_UP;
					break;
				case 'B':
					key = KEY_D_DOWN;
					break;
				case 'C':
					key = KEY_D_RIGHT;
					break;
				case 'D':
					key = KEY_D_LEFT;
					break;
				default:
				{
					for (size_t i = 0; i < strlen(tmp); i++)
					{
						if ((int)sizeof(UserInputBuffer) <= BackSpaceLimit)
							continue;

						append(UserInputBuffer, tmp[i]);
						BackSpaceLimit++;
						char keyBuf[2] = {(char)tmp[i], '\0'};
						ExPrint(keyBuf);
					}
					continue;
				}
				}

				ArrowInput(key);
				continue;
			}
		}

		if ((int)sizeof(UserInputBuffer) <= BackSpaceLimit)
			continue;

		append(UserInputBuffer, key);
		BackSpaceLimit++;
		char keyBuf[2] = {(char)key, '\0'};
		ExPrint(keyBuf);
	}
#endif
#endif // DEBUG
	debug("cpu halted");
	CPU::Halt(true);
}

nsa void DisplayStackSmashing()
{
	InitFont();
	ExPrint("\x1b[2J\x1b[H");
	Display->ClearBuffer();
	DisplayTopOverlay();

	ExPrint("\nWe're sorry, but the system has encountered a critical error and needs to restart.\n");
	ExPrint("\nError: Stack Smashing In Kernel Mode\n");

	CPUData *core = GetCurrentCPU();
	if (TaskManager)
	{
		ExPrint("Core: %d / pid: %d / tid: %d\n", core->ID,
				core->CurrentProcess->ID, core->CurrentThread->ID);
	}
	else if (core->IsActive)
		ExPrint("Core: %d\n", core->ID);

	ExPrint("\nWhat to do:\n");
	ExPrint(" This is a kernel bug.\n");
	ExPrint(" Please create a new issue on https://github.com/Fennix-Project/Fennix for further assistance.\n");

	/* TODO: Add additional info */
}

nsa void DisplayBufferOverflow()
{
	InitFont();
	ExPrint("\x1b[2J\x1b[H");
	Display->ClearBuffer();
	DisplayTopOverlay();

	ExPrint("\nWe're sorry, but the system has encountered a critical error and needs to restart.\n");
	ExPrint("\nError: Buffer Overflow In Kernel Mode\n");

	CPUData *core = GetCurrentCPU();
	if (TaskManager)
	{
		ExPrint("Core: %d / pid: %d / tid: %d\n", core->ID,
				core->CurrentProcess->ID, core->CurrentThread->ID);
	}
	else if (core->IsActive)
		ExPrint("Core: %d\n", core->ID);

	ExPrint("\nWhat to do:\n");
	ExPrint(" This is a kernel bug.\n");
	ExPrint(" Please create a new issue on https://github.com/Fennix-Project/Fennix for further assistance.\n");

	/* TODO: Add additional info */
}

nsa void DisplayAssertionFailed(const char *File, int Line, const char *Expression)
{
	InitFont();
	ExPrint("\x1b[2J\x1b[H");
	Display->ClearBuffer();
	DisplayTopOverlay();

	ExPrint("\nWe're sorry, but the system has encountered a critical error and needs to restart.\n");
	ExPrint("\nError: Assertion Failed\n");
	ExPrint("In file \"%s\" at line %d, \"%s\" failed\n",
			File, Line, Expression);

	CPUData *core = GetCurrentCPU();
	if (TaskManager && core->CurrentProcess.load() && core->CurrentThread.load())
	{
		ExPrint("Core: %d / pid: %d / tid: %d\n", core->ID,
				core->CurrentProcess->ID, core->CurrentThread->ID);
	}
	else if (core->IsActive)
		ExPrint("Core: %d\n", core->ID);

	ExPrint("\nWhat to do:\n");
	ExPrint(" This is a kernel bug.\n");
	ExPrint(" Please create a new issue on https://github.com/Fennix-Project/Fennix for further assistance.\n");

	CPU::ExceptionFrame ef;
	// Fill only the necessary fields

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"

	/* Jump over HandleAssertionFailed, and ip will be the function where it failed */
	void *fun = __builtin_return_address(1);
	/* Jump over this, HandleAssertionFailed & ip */
	void *stk = __builtin_frame_address(2);

#pragma GCC diagnostic pop

#ifdef __x86_64__
	asmv("movq %%cr3, %0" : "=r"(ef.cr3));
	ef.rip = (uint64_t)fun;
	ef.rbp = ef.rsp = (uint64_t)stk;
#elif defined(__i386__)
	asmv("movl %%cr3, %0" : "=r"(ef.cr3));
	ef.eip = (uint32_t)fun;
	ef.ebp = ef.esp = (uint32_t)stk;
#else
	UNUSED(fun);
	UNUSED(stk);
#endif
	DisplayStackScreen(&ef);

	/* TODO: Add additional info */
}

nsa void ArrowInput(uint8_t key)
{
	switch (key)
	{
	case KEY_D_RIGHT:
		if (ActiveScreen < 3)
			ActiveScreen++;
		else
			return;
		break;
	case KEY_D_LEFT:
		if (ActiveScreen > 0)
			ActiveScreen--;
		else
			return;
		break;
	case KEY_D_DOWN:
	case KEY_D_UP:
	default:
		return;
	}

	Display->ClearBuffer();
	ExPrint("\x1b[2J");
	DisplayTopOverlay();

	switch (ActiveScreen)
	{
	case 0:
		DisplayMainScreen(ExFrame);
		break;
	case 1:
		DisplayDetailsScreen(ExFrame);
		break;
	case 2:
		DisplayStackScreen(ExFrame);
		break;
	case 3:
		DisplayProcessScreen(ExFrame);
		break;
	default:
		break;
	}

	DisplayBottomOverlay();
}

nsa void UserInput(char *Input)
{
	debug("User input: %s", Input);
	Display->ClearBuffer();
	ExPrint("\x1b[2J");
	DisplayTopOverlay();

	if (strcmp(Input, "help") == 0)
	{
		ExPrint("Commands:\n");
		ExPrint("\x1b[1;37m  help              - Display this help message.\n");
		ExPrint("\x1b[0m  clear             - Clear the screen.\n");
		ExPrint("\x1b[1;37m  exit              - Shutdown the device.\n");
		ExPrint("\x1b[0m  reboot            - Reboot the device.\n");
		ExPrint("\x1b[1;37m  bitmap            - Display the kernel's bitmap.\n");
		ExPrint("\x1b[0m  mem               - Display memory information.\n");
		ExPrint("\x1b[1;37m  dump [addr] [len] - Dump [len] bytes from [addr].\n");
		ExPrint("\x1b[0m  diag              - Collect diagnostic information.\n");
		ExPrint("\x1b[1;37m  screen            - Display the final output prior to system panic.\n");
	}
	else if (strcmp(Input, "clear") == 0)
	{
		ExPrint("\x1b[2J");
		DisplayTopOverlay();
	}
	else if (strcmp(Input, "exit") == 0)
	{
		Display->ClearBuffer();

		const char msg[] = "Shutting down...";
		size_t msgLen = strlen(msg);
		size_t msgPixels = msgLen * CrashFontRenderer.CurrentFont->GetInfo().Width;
		uint32_t x = uint32_t((Display->GetWidth - msgPixels) / 2);
		uint32_t y = uint32_t((Display->GetHeight - CrashFontRenderer.CurrentFont->GetInfo().Height) / 2);
		x /= CrashFontRenderer.CurrentFont->GetInfo().Width;
		y /= CrashFontRenderer.CurrentFont->GetInfo().Height;
		ExPrint("\x1b[2J");
		ExPrint("\x1b[%d;%dH", y, x);
		ExPrint("\x1b[30;41m%s\x1b[0m\x1b[H", msg);

		PowerManager->Shutdown();
		CPU::Stop();
	}
	else if (strcmp(Input, "reboot") == 0)
	{
		Display->ClearBuffer();

		const char msg[] = "Rebooting...";
		size_t msgLen = strlen(msg);
		size_t msgPixels = msgLen * CrashFontRenderer.CurrentFont->GetInfo().Width;
		uint32_t x = uint32_t((Display->GetWidth - msgPixels) / 2);
		uint32_t y = uint32_t((Display->GetHeight - CrashFontRenderer.CurrentFont->GetInfo().Height) / 2);
		x /= CrashFontRenderer.CurrentFont->GetInfo().Width;
		y /= CrashFontRenderer.CurrentFont->GetInfo().Height;
		ExPrint("\x1b[2J");
		ExPrint("\x1b[%d;%dH", y, x);
		ExPrint("\x1b[30;41m%s\x1b[0m\x1b[H", msg);

		PowerManager->Reboot();
		CPU::Stop();
	}
	else if (strncmp(Input, "bitmap", 6) == 0)
	{
		Bitmap bm = KernelAllocator.GetPageBitmap();

		ExPrint("\n[0%%] %08ld: ", 0);
		for (size_t i = 0; i < bm.Size; i++)
		{
			if (bm.Get(i))
				ExPrint("\x1b[31m1\x1b[0m");
			else
				ExPrint("\x1b[32m0\x1b[0m");
			if (i % 128 == 127)
			{
				short Percentage = static_cast<short>((i * 100) / bm.Size);
				ExPrint("\n[%03ld%%] %08ld: ", Percentage, i);
			}
		}
		ExPrint("\n--- END OF BITMAP ---\nBitmap size: %ld\n\n.", bm.Size);
		DisplayTopOverlay();
	}
	else if (strcmp(Input, "mem") == 0)
	{
		uint64_t Total = KernelAllocator.GetTotalMemory();
		uint64_t Used = KernelAllocator.GetUsedMemory();
		uint64_t Free = KernelAllocator.GetFreeMemory();
		uint64_t Reserved = KernelAllocator.GetReservedMemory();

		ExPrint("\x1b[1;32mTotal: %ld bytes\n\x1b[1;31mUsed: %ld bytes\n\x1b[1;32mFree: %ld bytes\n\x1b[1;35mReserved: %ld bytes\n", Total, Used, Free, Reserved);
		int Progress = static_cast<int>((Used * 100) / Total);
		int ReservedProgress = static_cast<int>((Reserved * 100) / Total);
		ExPrint("\x1b[1;32m%3d%% \x1b[0m[", Progress);
		for (int i = 0; i < Progress; i++)
			ExPrint("\x1b[1;31m|");
		for (int i = 0; i < 100 - Progress; i++)
			ExPrint("\x1b[1;32m|");
		for (int i = 0; i < ReservedProgress; i++)
			ExPrint("\x1b[1;35m|");
		ExPrint("\x1b[0m]\n");
	}
	else if (strncmp(Input, "dump", 4) == 0)
	{
		char *arg = TrimWhiteSpace(Input + 4);
		char *addr = strtok(arg, " ");
		char *len = strtok(NULL, " ");
		if (addr == NULL || len == NULL)
		{
			ExPrint("\x1b[31mInvalid arguments\n");
			goto End;
		}

		uintptr_t Address = strtoul(addr, NULL, 16);
		size_t Length = strtoul(len, NULL, 10);

		uintptr_t AlignedAddress = ROUND_DOWN(Address, PAGE_SIZE);
		bool IsRangeValid = true;
		{
			Memory::Virtual vmm;
			for (uintptr_t adr = AlignedAddress;
				 adr < Address + Length;
				 adr += PAGE_SIZE)
			{
				if (!vmm.Check((void *)adr))
				{
					ExPrint("\x1b[31mAddress %#lx is not mapped\n", adr);
					IsRangeValid = false;
				}
			}
		}

		if (IsRangeValid)
		{
			debug("Dumping %ld bytes from %#lx\n", Length, Address);
			ExDumpData((void *)Address, (unsigned long)Length);
		}
	}
	else if (strcmp(Input, "diag") == 0)
	{
		DiagnosticDataCollection();
	}
	else if (strcmp(Input, "screen") == 0)
	{
		if (unlikely(FbBeforePanic == nullptr))
		{
			ExPrint("No screen data available\n");
			goto End;
		}
		memcpy(Display->GetBuffer, FbBeforePanic, Display->GetSize);
		return;
	}
#ifdef DEBUG
	else if (strcmp(Input, "pt") == 0)
	{
/* Helpful for qemu "info tlb" command */
#if defined(__amd64__) || defined(__i386__)
		CPU::PageTable((void *)ExFrame->cr3);
#endif
		ExPrint("Here be dragons\n");
	}
	else if (strcmp(Input, "ps") == 0)
	{
		DisplayProcessScreen(ExFrame, false);
	}
#endif // DEBUG
	else if (strlen(Input) > 0)
		ExPrint("Unknown command: %s", Input);
	else
		ExPrint("Use the 'help' command to display a list of available commands.");

End:
	DisplayBottomOverlay();
}
