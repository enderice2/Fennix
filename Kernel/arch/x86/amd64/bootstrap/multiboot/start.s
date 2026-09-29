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

.code32
KERNEL_STACK_SIZE = 0x4000 /* 16KB */

.extern DetectCPUID
.extern Detect64Bit
.extern DetectPSE
.extern DetectPAE
.extern multiboot_main
.extern LoadGDT32
.extern BootPageTable
.extern UpdatePageTable
.extern GDT64.Ptr
.extern GDT64.Code
.extern GDT64.Data

.section .bootstrap.data, "a"
MB_HeaderMagic:
	.quad 0

MB_HeaderInfo:
	.quad 0

.section .bootstrap.text, "a"

x32Hang:
	cli
	hlt
	jmp x32Hang

.global Multiboot_start
Multiboot_start:
	cli

	mov %eax, [MB_HeaderMagic]
	mov %ebx, [MB_HeaderInfo]

	call DetectCPUID
	cmp $0, %eax
	je x32Hang

	call Detect64Bit
	cmp $0, %eax
	je x32Hang

	call DetectPSE
	cmp $0, %eax
	je x32Hang

	call DetectPAE
	cmp $0, %eax
	je x32Hang

	mov %cr4, %ecx
	or $0x10, %ecx  /* PSE */
	or $0x20, %ecx  /* PAE */
	or $0x200, %ecx /* OSFXSR */
	or $0x400, %ecx /* OSXMMEXCPT */
	mov %ecx, %cr4

	call LoadGDT32
	call UpdatePageTable

	mov $BootPageTable, %ecx
	mov %ecx, %cr3

	mov $0xC0000080, %ecx /* EFER */
	rdmsr
	or $0x800, %eax /* LME */
	or $0x100, %eax /* LMA */
	or $0x1, %eax   /* SCE */
	wrmsr

	mov %cr0, %ecx
	and $~0x4, %ecx /* EM */
	or $0x2, %ecx   /* MP */
	or $0x80000000, %ecx /* PG */
	or $0x1, %ecx   /* PE */
	mov %ecx, %cr0

	lgdt [GDT64.Ptr]
	ljmp $GDT64.Code, $HigherHalfStart

.extern UpdatePageTable64

.code64
HigherHalfStart:
	mov $GDT64.Data, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	call UpdatePageTable64

	mov $(KernelStack + KERNEL_STACK_SIZE), %rsp
	mov $0x0, %rbp

	mov [MB_HeaderMagic], %rdi
	mov [MB_HeaderInfo], %rsi
	push %rsi
	push %rdi
	call multiboot_main
	.Hang:
		hlt
		jmp .Hang

.section .bootstrap.bss, "a"
.align 16
KernelStack:
	.space KERNEL_STACK_SIZE
