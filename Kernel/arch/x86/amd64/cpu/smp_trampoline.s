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

/* This has to be the same as enum SMPTrampolineAddress. */
TRAMPOLINE_PAGE_TABLE = 0x500
TRAMPOLINE_START_ADDR = 0x520
TRAMPOLINE_STACK = 0x570
TRAMPOLINE_GDT = 0x580
TRAMPOLINE_IDT = 0x590
TRAMPOLINE_CORE = 0x600
TRAMPOLINE_START = 0x2000

.section .rodata

/* ========== 16-bit ========== */

.code16
.global _trampoline_start
_trampoline_start:
	cli
	cld
	call Trampoline16

Trampoline16:
	mov $0x0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* Load Protected Mode GDT */
	lgdt [ProtectedMode_gdtr - _trampoline_start + TRAMPOLINE_START]

	/* Enable Protected Mode */
	mov %cr0, %eax
	or $0x1, %al
	mov %eax, %cr0

	/* Jump to Protected Mode */
	ljmp $0x8, $(Trampoline32 - _trampoline_start + TRAMPOLINE_START)

/* ========== 32-bit ========== */

.code32
Trampoline32:
	mov $0x10, %bx
	mov %bx, %ds
	mov %bx, %es
	mov %bx, %ss

	/* Set a page table */
	mov [TRAMPOLINE_PAGE_TABLE], %eax
	mov %eax, %cr3

	/* Enable PAE and PSE */
	mov %cr4, %eax
	or $0x20, %eax /* PAE */
	or $0x80, %eax /* PSE */
	mov %eax, %cr4

	/* Enable Long Mode */
	mov $0xC0000080, %ecx
	rdmsr
	or $0x100, %eax /* LME */
	wrmsr

	/* Enable paging */
	mov %cr0, %eax
	or $0x80000000, %eax /* PG */
	mov %eax, %cr0

	/* Load Long Mode GDT */
	lgdt [LongMode_gdtr - _trampoline_start + TRAMPOLINE_START]

	/* Jump to Long Mode */
	ljmp $0x8, $(Trampoline64 - _trampoline_start + TRAMPOLINE_START)

/* ========== 64-bit ========== */

.code64
Trampoline64:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss

	mov $0x0, %ax
	mov %ax, %fs
	mov %ax, %gs

	/* Set custom GDT & IDT */
	lgdt [TRAMPOLINE_GDT]
	lidt [TRAMPOLINE_IDT]

	/* Set up stack */
	mov [TRAMPOLINE_STACK], %rsp
	mov $0x0, %rbp

	/* Reset RFLAGS */
	push $0x0
	popf

	/* Jump to TrampolinePrepareExit */
	call TrampolineExit

.extern StartCPU
TrampolineExit:
	mov $StartCPU, %rax
	call *%rax

.align 16
ProtectedMode_gdtr:
	.word ProtectedModeGDTEnd - ProtectedModeGDTStart - 1
	.long ProtectedModeGDTStart - _trampoline_start + TRAMPOLINE_START

.align 16
ProtectedModeGDTStart:
	/* NULL segment */
	.quad 0x0

	/* Code segment */
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.word 0xCF9A
	.byte 0x00

	/* Data segment */
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.word 0xCF92
	.byte 0x00
ProtectedModeGDTEnd:
	nop

.align 16
LongMode_gdtr:
	.word LongModeGDTEnd - LongModeGDTStart - 1
	.quad LongModeGDTStart - _trampoline_start + TRAMPOLINE_START

.align 16
LongModeGDTStart:
	/* NULL segment */
	.quad 0x0

	/* Code segment */
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.word 0xAF98
	.byte 0x00

	/* Data segment */
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.word 0xCF92
	.byte 0x00
LongModeGDTEnd:
	nop

.global _trampoline_end
_trampoline_end:
