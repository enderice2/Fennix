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

PAGE_TABLE_SIZE = 0x4

.code32
.section .bootstrap.data, "a"
.align 0x1000
.global BootPageTable
BootPageTable:
	.space 0x10000 /* 0x4000 bytes will be used in UpdatePageTable */

.section .bootstrap.text, "a"
.global UpdatePageTable
UpdatePageTable:
	mov $(BootPageTable + 0x0000), %edi /* First PML4E */
	mov $(BootPageTable + 0x1000), %eax /* First PDPTE */
	or $0x3, %eax /* Bitwise OR on eax (PDPTE) with 11b (Present, Write) */
	mov %eax, (%edi) /* Write 11b to PML4E */

	mov $(BootPageTable + 0x1000), %edi /* First PDPTE */
	mov $(BootPageTable + 0x2000), %eax /* First PDE */
	or $0x3, %eax /* Bitwise OR on eax (PDE) with 11b (Present, Write) */

	mov $PAGE_TABLE_SIZE, %ecx /* For loop instruction */
	mov $0x0, %ebx /* Value to store in the next 4 bytes */
	.FillPageTableLevel3:
		mov %eax, (%edi) /* Store modified PDE in PDPTE */
		mov %ebx, 0x4(%edi) /* Store the ebx value in the next 4 bytes */
		add $0x1000, %eax /* Increment (page size) */
		adc $0x0, %ebx /* Add 0 to carry flag */
		add $0x8, %edi /* Add 8 to edi (next PDE) */
		loop .FillPageTableLevel3 /* Loop until ecx is 0 */

	mov $(BootPageTable + 0x2000), %edi /* First PDE */
	mov $0x83, %eax /* Present, Write, Large Page */

	mov $(512 * PAGE_TABLE_SIZE), %ecx /* For loop instruction */
	mov $0x0, %ebx /* Value to store in the next 4 bytes */
	.FillPageTableLevel2:
		mov %eax, (%edi) /* Store modified PDE in PDPTE */
		mov %ebx, 0x4(%edi) /* Store the ebx value in the next 4 bytes */
		add $0x200000, %eax /* Increment (page size) */
		adc $0x0, %ebx /* Add 0 (carry flag) to ebx to increment if there was a carry */
		add $0x8, %edi /* Add 8 to edi (next PDE) */
		loop .FillPageTableLevel2 /* Loop until ecx is 0 */

	ret
