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

.intel_syntax noprefix

.code32
.section .bootstrap.text, "a"

.global DetectCPUID
DetectCPUID:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 0x200000
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	xor eax, ecx
	jz .NoCPUID
	mov eax, 0x1
	ret
.NoCPUID:
	xor eax, eax
	ret

.global Detect64Bit
Detect64Bit:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb .NoLongMode
	mov eax, 0x80000001
	cpuid
	test edx, 0x20000000
	jz .NoLongMode
	mov eax, 0x1
	ret
.NoLongMode:
	xor eax, eax
	ret

.global DetectPSE
DetectPSE:
	mov eax, 0x00000001
	cpuid
	test edx, 0x00000008
	jz .NoPSE
	mov eax, 0x1
	ret
.NoPSE:
	xor eax, eax
	ret

.global DetectPAE
DetectPAE:
	mov eax, 0x00000001
	cpuid
	test edx, 0x00000040
	jz .NoPAE
	mov eax, 0x1
	ret
.NoPAE:
	xor eax, eax
	ret
