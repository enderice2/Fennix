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
.extern DetectPSE
.extern multiboot_main
.extern LoadGDT32
.extern BootPageTable

.section .bootstrap.data, "a"
MB_HeaderMagic:
	.quad 0

MB_HeaderInfo:
	.quad 0

.section .bootstrap.text, "a"

.global Multiboot_start
Multiboot_start:
	cli

	mov %eax, [MB_HeaderMagic]
	mov %ebx, [MB_HeaderInfo]

	call DetectCPUID
	cmp $0, %eax
	je .

	call DetectPSE
	cmp $0, %eax
	je .

	mov %cr4, %ecx
	or $0x00000010, %ecx /* PSE */
	mov %ecx, %cr4

	call LoadGDT32

	mov $BootPageTable, %ecx
	mov %ecx, %cr3

	mov %cr0, %ecx
	or $0x80000000, %ecx /* PG */
	mov %ecx, %cr0

	mov $(KernelStack + KERNEL_STACK_SIZE), %esp
	mov $0x0, %ebp

	mov [MB_HeaderMagic], %eax
	mov [MB_HeaderInfo], %ebx
	push %ebx
	push %eax
	call multiboot_main
.Hang:
	hlt
	jmp .Hang

.section .bootstrap.bss, "a"
.align 16
KernelStack:
	.space KERNEL_STACK_SIZE
