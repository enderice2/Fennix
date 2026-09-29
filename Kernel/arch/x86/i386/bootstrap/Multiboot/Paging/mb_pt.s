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
KERNEL_VIRTUAL_BASE = 0xC0000000 /* 3GB */
KERNEL_PAGE_NUMBER = 768 /* KERNEL_VIRTUAL_BASE >> 22 */

.section .bootstrap.data, "a"
.align 0x1000
.global BootPageTable
BootPageTable:
	.long 0x00000083
	.long 0x00400083
	.long 0x00800083
	.long 0x00C00083
	.long 0x01000083
	.long 0x01400083
	.long 0x01800083
	.long 0x01C00083
	.long 0x02000083
	.long 0x02400083
	.rept (KERNEL_PAGE_NUMBER - 10)
		.long 0
	.endr
	.long 0x00000083
	.long 0x00400083
	.long 0x00800083
	.long 0x00C00083
	.long 0x01000083
	.long 0x01400083
	.long 0x01800083
	.long 0x01C00083
	.long 0x02000083
	.long 0x02400083
	.rept (1024 - KERNEL_PAGE_NUMBER - 10)
		.long 0
	.endr
