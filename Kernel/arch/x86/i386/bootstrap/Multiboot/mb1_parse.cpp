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

#include <boot/protocol/multiboot.h>
#include <memory.hpp>

#include "../../../../kernel.h"

extern "C" void KernelEntry(struct BootInfo *Info);

void multiboot_parse(BootInfo &mb2binfo, uintptr_t Magic, uintptr_t Info)
{
	multiboot_info *InfoAddress = reinterpret_cast<multiboot_info *>(Info);

	if (InfoAddress->flags & MULTIBOOT_INFO_MEMORY)
	{
		fixme("mem_lower: %#x, mem_upper: %#x",
			  InfoAddress->mem_lower, InfoAddress->mem_upper);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_BOOTDEV)
	{
		fixme("boot_device: %#x",
			  InfoAddress->boot_device);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_CMDLINE)
	{
		strncpy(mb2binfo.Kernel.CommandLine,
				reinterpret_cast<const char *>(InfoAddress->cmdline),
				strlen(reinterpret_cast<const char *>(InfoAddress->cmdline)));
		debug("Kernel command line: %s", mb2binfo.Kernel.CommandLine);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_MODS)
	{
		multiboot_mod_list *module = reinterpret_cast<multiboot_mod_list *>(InfoAddress->mods_addr);
		for (size_t i = 0; i < InfoAddress->mods_count; i++)
		{
			if (i > MAX_MODULES)
			{
				warn("Too many modules, skipping the rest...");
				break;
			}
			mb2binfo.Modules[i].Address = (void *)(uint32_t)module[i].mod_start;
			mb2binfo.Modules[i].Size = module[i].mod_end - module[i].mod_start;
			strncpy(mb2binfo.Modules[i].Path, "(null)", 6);
			strncpy(mb2binfo.Modules[i].CommandLine, reinterpret_cast<const char *>(module[i].cmdline),
					strlen(reinterpret_cast<const char *>(module[i].cmdline)));
			debug("Module: %s", mb2binfo.Modules[i].Path);
		}
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_AOUT_SYMS)
	{
		fixme("aout_sym: [tabsize: %#x, strsize: %#x, addr: %#x, reserved: %#x]",
			  InfoAddress->u.aout_sym.tabsize, InfoAddress->u.aout_sym.strsize,
			  InfoAddress->u.aout_sym.addr, InfoAddress->u.aout_sym.reserved);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_ELF_SHDR)
	{
		mb2binfo.Kernel.Symbols.Num = InfoAddress->u.elf_sec.num;
		mb2binfo.Kernel.Symbols.EntSize = InfoAddress->u.elf_sec.size;
		mb2binfo.Kernel.Symbols.Shndx = InfoAddress->u.elf_sec.shndx;
		mb2binfo.Kernel.Symbols.Sections = static_cast<uintptr_t>(InfoAddress->u.elf_sec.addr);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		mb2binfo.Memory.Entries = InfoAddress->mmap_length / sizeof(multiboot_mmap_entry);
		for (uint32_t i = 0; i < mb2binfo.Memory.Entries; i++)
		{
			if (i > MAX_MEMORY_ENTRIES)
			{
				warn("Too many memory entries, skipping the rest...");
				break;
			}
			multiboot_mmap_entry entry = reinterpret_cast<multiboot_mmap_entry *>(InfoAddress->mmap_addr)[i];
			mb2binfo.Memory.Size += (__SIZE_TYPE__)entry.len;
			switch (entry.type)
			{
			case MULTIBOOT_MEMORY_AVAILABLE:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = Usable;
				break;
			case MULTIBOOT_MEMORY_RESERVED:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = Reserved;
				break;
			case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = ACPIReclaimable;
				break;
			case MULTIBOOT_MEMORY_NVS:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = ACPINVS;
				break;
			case MULTIBOOT_MEMORY_BADRAM:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = BadMemory;
				break;
			default:
				mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
				mb2binfo.Memory.Entry[i].Length = (__SIZE_TYPE__)entry.len;
				mb2binfo.Memory.Entry[i].Type = Unknown;
				break;
			}
			debug("Memory entry: [BaseAddress: %#x, Length: %#x, Type: %d]",
				  mb2binfo.Memory.Entry[i].BaseAddress,
				  mb2binfo.Memory.Entry[i].Length,
				  mb2binfo.Memory.Entry[i].Type);
		}
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_DRIVE_INFO)
	{
		fixme("drives_length: %d, drives_addr: %#x",
			  InfoAddress->drives_length, InfoAddress->drives_addr);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_CONFIG_TABLE)
	{
		fixme("config_table: %#x",
			  InfoAddress->config_table);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
	{
		strncpy(mb2binfo.Bootloader.Name,
				reinterpret_cast<const char *>(InfoAddress->boot_loader_name),
				strlen(reinterpret_cast<const char *>(InfoAddress->boot_loader_name)));
		debug("Bootloader name: %s", mb2binfo.Bootloader.Name);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_APM_TABLE)
	{
		fixme("apm_table: %#x",
			  InfoAddress->apm_table);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_VBE_INFO)
	{
		fixme("vbe_control_info: %#x, vbe_mode_info: %#x, vbe_mode: %#x, vbe_interface_seg: %#x, vbe_interface_off: %#x, vbe_interface_len: %#x",
			  InfoAddress->vbe_control_info, InfoAddress->vbe_mode_info,
			  InfoAddress->vbe_mode, InfoAddress->vbe_interface_seg,
			  InfoAddress->vbe_interface_off, InfoAddress->vbe_interface_len);
	}
	if (InfoAddress->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
	{
		static int fb_count = 0;
		mb2binfo.Framebuffer[fb_count].BaseAddress = (void *)InfoAddress->framebuffer_addr;
		mb2binfo.Framebuffer[fb_count].Width = InfoAddress->framebuffer_width;
		mb2binfo.Framebuffer[fb_count].Height = InfoAddress->framebuffer_height;
		mb2binfo.Framebuffer[fb_count].Pitch = InfoAddress->framebuffer_pitch;
		mb2binfo.Framebuffer[fb_count].BitsPerPixel = InfoAddress->framebuffer_bpp;
		switch (InfoAddress->framebuffer_type)
		{
		case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
		{
			mb2binfo.Framebuffer[fb_count].Type = Indexed;
			break;
		}
		case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
		{
			mb2binfo.Framebuffer[fb_count].Type = RGB;
			mb2binfo.Framebuffer[fb_count].RedMaskSize = InfoAddress->framebuffer_red_mask_size;
			mb2binfo.Framebuffer[fb_count].RedMaskShift = InfoAddress->framebuffer_red_field_position;
			mb2binfo.Framebuffer[fb_count].GreenMaskSize = InfoAddress->framebuffer_green_mask_size;
			mb2binfo.Framebuffer[fb_count].GreenMaskShift = InfoAddress->framebuffer_green_field_position;
			mb2binfo.Framebuffer[fb_count].BlueMaskSize = InfoAddress->framebuffer_blue_mask_size;
			mb2binfo.Framebuffer[fb_count].BlueMaskShift = InfoAddress->framebuffer_blue_field_position;
			break;
		}
		case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
		{
			mb2binfo.Framebuffer[fb_count].Type = EGA;
			break;
		}
		default:
		{
			mb2binfo.Framebuffer[fb_count].Type = Unknown_Framebuffer_Type;
			break;
		}
		}
		debug("Framebuffer %d: %dx%d %d bpp",
			  fb_count, InfoAddress->framebuffer_width,
			  InfoAddress->framebuffer_height,
			  InfoAddress->framebuffer_bpp);
		debug("More info:\nAddress: %p\nPitch: %d\nMemoryModel: %d\nRedMaskSize: %d\nRedMaskShift: %d\nGreenMaskSize: %d\nGreenMaskShift: %d\nBlueMaskSize: %d\nBlueMaskShift: %d",
			  InfoAddress->framebuffer_addr, InfoAddress->framebuffer_pitch, InfoAddress->framebuffer_type,
			  InfoAddress->framebuffer_red_mask_size, InfoAddress->framebuffer_red_field_position, InfoAddress->framebuffer_green_mask_size,
			  InfoAddress->framebuffer_green_field_position, InfoAddress->framebuffer_blue_mask_size, InfoAddress->framebuffer_blue_field_position);
	}

	mb2binfo.Kernel.PhysicalBase = (void *)&_bootstrap_start;
	mb2binfo.Kernel.VirtualBase = (void *)(uint32_t)((uint32_t)&_bootstrap_start + 0xC0000000);
	mb2binfo.Kernel.Size = ((uint32_t)&_kernel_end - (uint32_t)&_kernel_start) + ((uint32_t)&_bootstrap_end - (uint32_t)&_bootstrap_start);
	debug("Kernel base: %p (physical) %p (virtual)", mb2binfo.Kernel.PhysicalBase, mb2binfo.Kernel.VirtualBase);

	KernelEntry(&mb2binfo);
}
