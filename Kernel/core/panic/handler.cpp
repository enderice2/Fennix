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
#include <kcon.hpp>
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

using namespace KernelConsole;

extern const char *x86ExceptionMnemonics[];
extern void DisplayCrashScreen(CPU::ExceptionFrame *);
extern bool UserModeExceptionHandler(CPU::ExceptionFrame *);
extern void DisplayStackSmashing();
extern void DisplayBufferOverflow();
extern void DisplayAssertionFailed(const char *, int, const char *);

void *FbBeforePanic = nullptr;
size_t FbPagesBeforePanic = 0;
FontRenderer CrashFontRenderer;

static int ExTermColors[] = {
	[TerminalColor::BLACK] = 0x000000,
	[TerminalColor::RED] = 0xAA0000,
	[TerminalColor::GREEN] = 0x00AA00,
	[TerminalColor::YELLOW] = 0xAA5500,
	[TerminalColor::BLUE] = 0x0000AA,
	[TerminalColor::MAGENTA] = 0xAA00AA,
	[TerminalColor::CYAN] = 0x00AAAA,
	[TerminalColor::GREY] = 0xAAAAAA,
};

static int ExTermBrightColors[] = {
	[TerminalColor::BLACK] = 0x858585,
	[TerminalColor::RED] = 0xFF5555,
	[TerminalColor::GREEN] = 0x55FF55,
	[TerminalColor::YELLOW] = 0xFFFF55,
	[TerminalColor::BLUE] = 0x5555FF,
	[TerminalColor::MAGENTA] = 0xFF55FF,
	[TerminalColor::CYAN] = 0x55FFFF,
	[TerminalColor::GREY] = 0xFFFFFF,
};

void paint_callback(TerminalCell *cell, long x, long y)
{
	if (cell->attr.Bright)
		CrashFontRenderer.Paint(x, y, cell->c, ExTermBrightColors[cell->attr.Foreground], ExTermColors[cell->attr.Background]);
	else
		CrashFontRenderer.Paint(x, y, cell->c, ExTermColors[cell->attr.Foreground], ExTermColors[cell->attr.Background]);
}

nsa void __printfWrapper(char c, void *)
{
	KernelConsole::Terminals[15]->Term->Process(c);
}

nsa void ExPrint(const char *Format, ...)
{
	va_list args;
	va_start(args, Format);
	vfctprintf(__printfWrapper, NULL, Format, args);
	va_end(args);
}

nsa void HaltAllCores()
{
	if (SMP::CPUCores <= 1)
		return;

#if defined(__amd64__)
	fixme("HALT ALL CORES");
	// if (Interrupts::apic[0] == nullptr)
	// 	return;

	// APIC::InterruptCommandRegister icr{};

	// bool x2APIC = ((APIC::APIC *)Interrupts::apic[0])->x2APIC;
	// if (likely(x2APIC))
	// {
	// 	icr.x2.VEC = static_cast<uint8_t>(CPU::x86::IRQ31);
	// 	icr.x2.MT = APIC::Fixed;
	// 	icr.x2.L = APIC::Assert;

	// 	for (int i = 1; i < SMP::CPUCores; i++)
	// 	{
	// 		icr.x2.DES = uint8_t(i);
	// 		((APIC::APIC *)Interrupts::apic[i])->ICR(icr);
	// 	}
	// }
	// else
	// {
	// 	icr.VEC = static_cast<uint8_t>(CPU::x86::IRQ31);
	// 	icr.MT = APIC::Fixed;
	// 	icr.L = APIC::Assert;

	// 	for (int i = 1; i < SMP::CPUCores; i++)
	// 	{
	// 		icr.DES = uint8_t(i);
	// 		((APIC::APIC *)Interrupts::apic[i])->ICR(icr);
	// 	}
	// }
#elif defined(__aarch64__)
#endif
}

nsa void InitFont()
{
	/* Hope we won't crash here */

	if (Display == nullptr)
	{
		error("Can't initialize font without display initalized");
		CPU::Stop();
	}

	if (FbBeforePanic == nullptr)
	{
		FbPagesBeforePanic = TO_PAGES(Display->GetSize);
		FbBeforePanic = KernelAllocator.RequestPages(FbPagesBeforePanic);
		debug("Created before panic framebuffer at %#lx", FbBeforePanic);
	}
	memcpy(FbBeforePanic, Display->GetBuffer, Display->GetSize);

	if (CrashFontRenderer.CurrentFont == nullptr)
		CrashFontRenderer.CurrentFont = new Video::Font(&_binary_files_tamsyn_font_1_11_Tamsyn8x16b_psf_start,
														&_binary_files_tamsyn_font_1_11_Tamsyn8x16b_psf_end,
														Video::FontType::PCScreenFont2);

	if (Terminals[15] == nullptr)
	{
		size_t Cols = Display->GetWidth / CrashFontRenderer.CurrentFont->GetInfo().Width;
		size_t Rows = Display->GetHeight / CrashFontRenderer.CurrentFont->GetInfo().Height;
		Terminals[15] = new ConsoleTerminal;
		Terminals[15]->Term = new VirtualTerminal(Cols, Rows, Display->GetWidth, Display->GetHeight, paint_callback, nullptr);
	}
}

std::atomic<bool> UnrecoverableLock = false;
nsa __noreturn void HandleUnrecoverableException(CPU::ExceptionFrame *Frame)
{
	static int setOnce = 0;
	if (!setOnce++) /* FIXME: SMP */
	{
		for (uint32_t x = 0; x < Display->GetWidth; x++)
			for (uint32_t y = 0; y < Display->GetHeight; y++)
				Display->SetPixel(x, y, 0xFF250500);

		ExPrint("\x1b[H");
	}

	CPUData *core = GetCurrentCPU();

	while (UnrecoverableLock.exchange(true, std::memory_order_acquire))
		CPU::Pause();

	ExPrint("\x1b[0m-----------------------------------------------\n");
#if defined(__amd64__) || defined(__i386__)
	ExPrint("\x1b[30;41mUnrecoverable exception %#lx on CPU %d\n",
			Frame->InterruptNumber, core->ID);
#else
	UNUSED(core);
#endif
#if defined(__amd64__) || defined(__i386__)
	ExPrint("CR0=%#lx CR2=%#lx CR3=%#lx CR4=%#lx CR8=%#lx\n",
			Frame->cr0, Frame->cr2, Frame->cr3, Frame->cr4, Frame->cr8);
	ExPrint("DR0=%#lx DR1=%#lx DR2=%#lx DR3=%#lx DR6=%#lx DR7=%#lx\n",
			Frame->dr0, Frame->dr1, Frame->dr2, Frame->dr3, Frame->dr6, Frame->dr7);
	ExPrint("GS=%#lx FS=%#lx ES=%#lx DS=%#lx SS=%#lx CS=%#lx\n",
			Frame->gs, Frame->fs, Frame->es, Frame->ds, Frame->ss, Frame->cs);
#endif
#if defined(__amd64__)
	ExPrint("R8=%#lx R9=%#lx R10=%#lx R11=%#lx R12=%#lx R13=%#lx R14=%#lx R15=%#lx\n",
			Frame->r8, Frame->r9, Frame->r10, Frame->r11, Frame->r12, Frame->r13,
			Frame->r14, Frame->r15);
#endif
#if defined(__amd64__) || defined(__i386__)
	ExPrint("AX=%#lx BX=%#lx CX=%#lx DX=%#lx SI=%#lx DI=%#lx BP=%#lx SP=%#lx\n",

#ifdef __amd64__
			Frame->rax, Frame->rbx, Frame->rcx, Frame->rdx, Frame->rsi, Frame->rdi,
			Frame->rbp, Frame->rsp);
#else
			Frame->eax, Frame->ebx, Frame->ecx, Frame->edx, Frame->esi, Frame->edi,
			Frame->ebp, Frame->esp);
#endif /* __amd64__ */

	ExPrint("IP=%#lx FL=%#lx INT=%#lx ERR=%#lx\n",

#ifdef __amd64__
			Frame->rip, Frame->rflags.raw,
#else
			Frame->eip, Frame->eflags.raw,
#endif /* __amd64__ */
			Frame->InterruptNumber, Frame->ErrorCode);
#endif /* a86 */

	Display->UpdateBuffer();
#if defined(__amd64__) || defined(__i386__)
	error("Unrecoverable Exception: %#lx", Frame->InterruptNumber);
#endif

	UnrecoverableLock.store(false, std::memory_order_release);
	CPU::Stop();
}

nsa __noreturn void HandleExceptionInsideException(CPU::ExceptionFrame *Frame)
{
	ExPrint("\x1b[0m-----------------------------------------------\n");
#if defined(__amd64__) || defined(__i386__)
	ExPrint("Exception inside exception: %#lx at %#lx\n",
			Frame->InterruptNumber,
#if defined(__amd64__)
			Frame->rip);
#elif defined(__i386__)
			Frame->eip);
#elif defined(__aarch64__)
			Frame->pc);
#endif
#endif // __amd64__ || __i386__
#if defined(__amd64__) || defined(__i386__)
	ExPrint("CR0=%#lx CR2=%#lx CR3=%#lx CR4=%#lx CR8=%#lx\n",
			Frame->cr0, Frame->cr2, Frame->cr3, Frame->cr4, Frame->cr8);
	ExPrint("DR0=%#lx DR1=%#lx DR2=%#lx DR3=%#lx DR6=%#lx DR7=%#lx\n",
			Frame->dr0, Frame->dr1, Frame->dr2, Frame->dr3, Frame->dr6, Frame->dr7);
	ExPrint("GS=%#lx FS=%#lx ES=%#lx DS=%#lx SS=%#lx CS=%#lx\n",
			Frame->gs, Frame->fs, Frame->es, Frame->ds, Frame->ss, Frame->cs);
#endif
#if defined(__amd64__)
	ExPrint("R8=%#lx R9=%#lx R10=%#lx R11=%#lx R12=%#lx R13=%#lx R14=%#lx R15=%#lx\n",
			Frame->r8, Frame->r9, Frame->r10, Frame->r11, Frame->r12, Frame->r13,
			Frame->r14, Frame->r15);
#endif
#if defined(__amd64__) || defined(__i386__)
	ExPrint("AX=%#lx BX=%#lx CX=%#lx DX=%#lx SI=%#lx DI=%#lx BP=%#lx SP=%#lx\n",

#ifdef __amd64__
			Frame->rax, Frame->rbx, Frame->rcx, Frame->rdx, Frame->rsi, Frame->rdi,
			Frame->rbp, Frame->rsp);
#else
			Frame->eax, Frame->ebx, Frame->ecx, Frame->edx, Frame->esi, Frame->edi,
			Frame->ebp, Frame->esp);
#endif /* __amd64__ */

	ExPrint("IP=%#lx FL=%#lx INT=%#lx ERR=%#lx\n",

#ifdef __amd64__
			Frame->rip, Frame->rflags.raw,
#else
			Frame->eip, Frame->eflags.raw,
#endif /* __amd64__ */
			Frame->InterruptNumber, Frame->ErrorCode);
#endif /* a86 */
	Display->UpdateBuffer();
	CPU::Stop();
}

std::atomic<bool> ExceptionLock = false;
nsa void HandleException(CPU::ExceptionFrame *Frame)
{
	/* We don't need to restore the page table
		because the ExceptionHandlerStub will
		do it for us if we return. */
	CPU::PageTable(KernelPageTable);
	InitFont();

#if defined(__amd64__) || defined(__i386__)

	/* First check the exception */
	if (unlikely(Frame->InterruptNumber == CPU::x86::DoubleFault))
	{
		ExceptionLock.store(true, std::memory_order_release);
		HandleUnrecoverableException(Frame);
	}

	if (Frame->cs == GDT_USER_CODE && Frame->ss == GDT_USER_DATA)
	{
		if (UserModeExceptionHandler(Frame))
			goto ExceptionExit;

		CPUData *core = GetCurrentCPU();
		if (likely(core->CurrentThread->Security.IsCritical == false))
			TaskManager->Yield();

		error("Critical thread \"%s\"(%d) died",
			  core->CurrentThread->Name,
			  core->CurrentThread->ID);
	}

	debug("-----------------------------------------------------------------------------------");
	error("Exception: %#x", Frame->InterruptNumber);
	debug("%ld MiB / %ld MiB (%ld MiB Reserved)",
		  TO_MiB(KernelAllocator.GetUsedMemory()),
		  TO_MiB(KernelAllocator.GetTotalMemory()),
		  TO_MiB(KernelAllocator.GetReservedMemory()));

	if (ExceptionLock.load(std::memory_order_acquire))
		HandleExceptionInsideException(Frame);
	ExceptionLock.store(true, std::memory_order_release);

	{
		const char msg[] = "Entering in panic mode...";
		size_t msgLen = strlen(msg);
		size_t msgPixels = msgLen * CrashFontRenderer.CurrentFont->GetInfo().Width;
		uint32_t x = uint32_t((Display->GetWidth - msgPixels) / 2);
		uint32_t y = uint32_t((Display->GetHeight - CrashFontRenderer.CurrentFont->GetInfo().Height) / 2);
		x /= CrashFontRenderer.CurrentFont->GetInfo().Width;
		y /= CrashFontRenderer.CurrentFont->GetInfo().Height;
		printf("\x1b[2J\x1b[%d;%dH", y, x);
		printf("\x1b[30;41m%s\x1b[0m\x1b[H", msg);
	}

	if (DriverManager)
		DriverManager->Panic();
	if (TaskManager)
		TaskManager->Panic();
	irq.UnregisterAllHandlers();
	HaltAllCores();
	ForceUnlock = true;

	DisplayCrashScreen(Frame);
	CPU::Stop();

ExceptionExit:
	ExceptionLock.store(false, std::memory_order_release);
#endif
}

nsa void BaseBufferStackError(bool Stack)
{
	/* We don't need to restore the page table
		because the ExceptionHandlerStub will
		do it for us if we return. */
	CPU::PageTable(KernelPageTable);

	if (CrashFontRenderer.CurrentFont == nullptr)
		CrashFontRenderer.CurrentFont = new Video::Font(&_binary_files_tamsyn_font_1_11_Tamsyn8x16b_psf_start,
														&_binary_files_tamsyn_font_1_11_Tamsyn8x16b_psf_end,
														Video::FontType::PCScreenFont2);

	if (Terminals[15] == nullptr)
	{
		size_t Cols = Display->GetWidth / CrashFontRenderer.CurrentFont->GetInfo().Width;
		size_t Rows = Display->GetHeight / CrashFontRenderer.CurrentFont->GetInfo().Height;
		Terminals[15] = new ConsoleTerminal;
		Terminals[15]->Term = new VirtualTerminal(Cols, Rows, Display->GetWidth, Display->GetHeight, paint_callback, nullptr);
	}

	ExceptionLock.store(true, std::memory_order_release);

	if (DriverManager)
		DriverManager->Panic();
	if (TaskManager)
		TaskManager->Panic();
	HaltAllCores();
	ForceUnlock = true;

	debug("-----------------------------------------------------------------------------------");
	error("%s", Stack ? "Stack smashing detected" : "Buffer overflow detected");
	debug("%ld MiB / %ld MiB (%ld MiB Reserved)",
		  TO_MiB(KernelAllocator.GetUsedMemory()),
		  TO_MiB(KernelAllocator.GetTotalMemory()),
		  TO_MiB(KernelAllocator.GetReservedMemory()));
}

nsa __noreturn void HandleStackSmashing()
{
	BaseBufferStackError(true);
	DisplayStackSmashing();
	CPU::Stop();
}

nsa __noreturn void HandleBufferOverflow()
{
	BaseBufferStackError(false);
	DisplayBufferOverflow();
	CPU::Stop();
}

EXTERNC nsa __noreturn void HandleAssertionFailed(const char *File, int Line, const char *Expression)
{
	DisplayAssertionFailed(File, Line, Expression);
	CPU::Stop();
}
