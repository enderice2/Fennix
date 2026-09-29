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

/* https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html */
.section .multiboot2, "a"
.align 0x1000
MULTIBOOT2_HEADER_START:
	.long 0xE85250D6
	.long 0
	.long (MULTIBOOT2_HEADER_END - MULTIBOOT2_HEADER_START)
	.long 0x100000000 - (MULTIBOOT2_HEADER_END - MULTIBOOT2_HEADER_START) - 0 - 0xE85250D6
.align 8
InfoRequestTag_Start:
	.word 1
	.word 0
	.long InfoRequestTag_End - InfoRequestTag_Start
	.long 1 /* Command Line */
	.long 2 /* Boot Loader Name */
	.long 3 /* Module */
	.long 4 /* Basic Memory Information */
	.long 5 /* BIOS Boot Device */
	.long 6 /* Memory Map */
	.long 7 /* VBE */
	.long 8 /* Framebuffer */
	.long 9 /* ELF Sections */
	.long 10 /* APM Table */
	.long 11 /* EFI 32-bit System Table Pointer */
	.long 12 /* EFI 64-bit System Table Pointer */
	/* .long 13 */ /* SMBIOS */
	.long 14 /* ACPI Old */
	.long 15 /* ACPI New */
	.long 16 /* Network */
	.long 17 /* EFI Memory Map */
	.long 18 /* EFI Boot Services Notifier */
	.long 19 /* EFI 32-bit Image Handle Pointer */
	.long 20 /* EFI 64-bit Image Handle Pointer */
	.long 21 /* Load Base Address */
InfoRequestTag_End:
.align 8
FramebufferTag_Start:
	.word 5
	.word 1
	.long FramebufferTag_End - FramebufferTag_Start
	.long 0
	.long 0
	.long 32
FramebufferTag_End:
.align 8
EGATextSupportTag_Start:
	.word 4
	.word 0
	.long EGATextSupportTag_End - EGATextSupportTag_Start
	.long 0 /* https://www.gnu.org/software/grub/manual/multiboot2/html_node/Console-header-tags.html */
EGATextSupportTag_End:
.align 8
AlignedModulesTag_Start:
	.word 6
	.word 0
	.long AlignedModulesTag_End - AlignedModulesTag_Start
AlignedModulesTag_End:
.align 8
EntryAddressTag_Start:
	.word 3
	.word 0
	.long EntryAddressTag_End - EntryAddressTag_Start
	.long Multiboot_start
EntryAddressTag_End:
.align 8
EndTag_Start:
	.word 0
	.word 0
	.long EndTag_End - EndTag_Start
EndTag_End:
MULTIBOOT2_HEADER_END:
	nop
