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

#include <types.h>

#include <memory.hpp>

#include "../../../../kernel.h"

#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

void multiboot_parse(BootInfo &mb2binfo, uintptr_t Magic, uintptr_t Info);
void multiboot2_parse(BootInfo &mb2binfo, uintptr_t Magic, uintptr_t Info);

EXTERNC void multiboot_main(uintptr_t Magic, uintptr_t Info)
{
	BootInfo mb2binfo{};

	if (Info == NULL || Magic == NULL)
	{
		if (Magic == NULL)
			error("Multiboot magic is NULL");
		if (Info == NULL)
			error("Multiboot info is NULL");
		CPU::Stop();
	}
	else if (Magic == MULTIBOOT_BOOTLOADER_MAGIC)
		multiboot_parse(mb2binfo, Magic, Info);
	else if (Magic == MULTIBOOT2_BOOTLOADER_MAGIC)
		multiboot2_parse(mb2binfo, Magic, Info);
	else
	{
		error("Unknown multiboot magic %#x", Magic);
		CPU::Stop();
	}
}
