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

.section ".bootstrap.boot", "a"

.extern _kernel_bss_start
.extern _kernel_bss_end
.extern _kernel_bss_size

.global _start
_start:
	/* Keep only the main core */
	mrs x1, mpidr_el1
	and x1, x1, #3
	cbz x1, 2f

	/* Halt */
1:	wfe
	b 1b

	/* Initialize the stack */
2:	ldr x1, =_start
	mov sp, x1

	/* Clear the BSS */
	ldr x1, =_kernel_bss_start
	ldr w2, =_kernel_bss_size
3:	cbz w2, 4f
	str xzr, [x1], #8
	sub w2, w2, #1
	cbnz w2, 3b

	/* Start the kernel */
4:	bl _aarch64_start
	b 1b
