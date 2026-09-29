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

.code64
.section .bootstrap.data, "a"

/* Access bits */
A = 0x1
RW = 0x2
DC = 0x4
E = 0x8
S = 0x10
DPL0 = 0x0 /* 0 << 5 ???? */
DPL1 = 0x20
P = 0x80

/* Flags bits */
LONG_MODE = 0x20
SZ_32 = 0x40
GRAN_4K = 0x80

.global GDT64.Null
.global GDT64.Code
.global GDT64.Data
.global GDT64.Tss
.global GDT64.Ptr

GDT64:
GDT64.Null = . - GDT64
	.quad 0
GDT64.Code = . - GDT64
	.long 0xFFFF
	.byte 0
	.byte P | S | E | RW
	.byte GRAN_4K | LONG_MODE | 0xF
	.byte 0
GDT64.Data = . - GDT64
	.long 0xFFFF
	.byte 0
	.byte P | S | RW
	.byte GRAN_4K | SZ_32 | 0xF
	.byte 0
GDT64.Tss = . - GDT64
	.long 0x00000068
	.long 0x00CF8900
GDT64.Ptr:
	.word . - GDT64 - 1
	.quad GDT64
