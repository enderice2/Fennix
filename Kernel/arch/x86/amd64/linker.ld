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

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PF_R = 0x4;
PF_W = 0x2;
PF_X = 0x1;

PHDRS
{
	bootstrap   PT_LOAD    FLAGS( PF_R | PF_W /*| PF_X*/ );
	text        PT_LOAD    FLAGS( PF_R | PF_X );
	data        PT_LOAD    FLAGS( PF_R | PF_W );
	rodata      PT_LOAD    FLAGS( PF_R );
	bss         PT_LOAD    FLAGS( PF_R | PF_W );
}

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
	. = 0x100000;
	_bootstrap_start = .;
	.bootstrap ALIGN(CONSTANT(MAXPAGESIZE)) :
	{
		*(.multiboot)
		*(.multiboot2)
		*(.bootstrap .bootstrap.*)
	} :bootstrap
	_bootstrap_end = ALIGN(CONSTANT(MAXPAGESIZE));
	. += KERNEL_VMA;

	/* . += KERNEL_VMA;
	_bootstrap_start = .;
    .limine_requests : {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :bootstrap
	_bootstrap_end = ALIGN(CONSTANT(MAXPAGESIZE)); */

	_kernel_start = ALIGN(CONSTANT(MAXPAGESIZE));
	_kernel_text_start = ALIGN(CONSTANT(MAXPAGESIZE));
	.text ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.text) - KERNEL_VMA)
	{
		*(.text .text.*)
	} :text
	_kernel_text_end = ALIGN(CONSTANT(MAXPAGESIZE));

	_kernel_data_start = ALIGN(CONSTANT(MAXPAGESIZE));
	.data ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.data) - KERNEL_VMA)
	{
		*(.data .data.*)
	} :data

	.eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) ONLY_IF_RW
	{
		KEEP (*(.eh_frame .eh_frame.*))
	} :data

	.gcc_except_table : AT(ADDR(.gcc_except_table) - KERNEL_VMA) ONLY_IF_RW
	{
		KEEP (*(.gcc_except_table .gcc_except_table.*))
	} :data
	_kernel_data_end = ALIGN(CONSTANT(MAXPAGESIZE));

	_kernel_rodata_start = ALIGN(CONSTANT(MAXPAGESIZE));
	.rodata ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.rodata) - KERNEL_VMA)
	{
		*(.rodata .rodata.*)
	} :rodata

	.builtin_drivers ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.builtin_drivers) - KERNEL_VMA)
	{
		__kernel_builtin_drivers_start = .;
		KEEP(*(SORT(.builtin_drivers.*)))
		KEEP(*(.builtin_drivers))
		__kernel_builtin_drivers_end = .;
	} :rodata

	.init_array ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.init_array) - KERNEL_VMA)
	{
		PROVIDE_HIDDEN(__init_array_start = .);
		KEEP(*(.init_array .ctors))
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		PROVIDE_HIDDEN (__init_array_end = .);
	} :rodata

	.fini_array ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.fini_array) - KERNEL_VMA)
	{
		PROVIDE_HIDDEN(__fini_array_start = .);
		KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP(*(.fini_array .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} :rodata

	.eh_frame_hdr : AT(ADDR(.eh_frame_hdr) - KERNEL_VMA)
	{
		*(.eh_frame_hdr .eh_frame_hdr.*)
	} :rodata

	.eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) ONLY_IF_RO
	{
		KEEP (*(.eh_frame .eh_frame.*))
	} :rodata

	.gcc_except_table : AT(ADDR(.gcc_except_table) - KERNEL_VMA) ONLY_IF_RO
	{
		KEEP (*(.gcc_except_table .gcc_except_table.*))
	} :rodata
	_kernel_rodata_end = ALIGN(CONSTANT(MAXPAGESIZE));

	_kernel_bss_start = ALIGN(CONSTANT(MAXPAGESIZE));
	.bss ALIGN(CONSTANT(MAXPAGESIZE)) : AT(ADDR(.bss) - KERNEL_VMA)
	{
		*(COMMON)
		*(.bss .bss.*)
	} :bss
	_kernel_bss_end = ALIGN(CONSTANT(MAXPAGESIZE));
	_kernel_end = ALIGN(CONSTANT(MAXPAGESIZE));

	/DISCARD/ :
	{
		*(.comment*)
	}
}
