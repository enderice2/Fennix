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
.extern Multiboot_start

.section .multiboot, "a"
.align 4

MULTIBOOT_HEADER:
	.long 0x1BADB002
	.long 0x1 | 0x2 | 0x4
	.long -(0x1BADB002 + (0x1 | 0x2 | 0x4))
	/* KLUDGE */
	.long 0
	.long 0
	.long 0
	.long 0
	.long 0
	/* VIDEO MODE */
	.long 0
	.long 0
	.long 0
	.long 0
