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

#include <boot/protocol/multiboot2.h>
#include <memory.hpp>

#include "../../../../kernel.h"

extern "C" void KernelEntry(struct BootInfo *Info);

void multiboot2_parse(BootInfo &mb2binfo, uintptr_t Magic, uintptr_t Info)
{
	auto infoAddr = Info;
	for (auto Tag = (struct multiboot_tag *)((uint8_t *)infoAddr + 8);; Tag = (struct multiboot_tag *)((multiboot_uint8_t *)Tag + ((Tag->size + 7) & ~7)))
	{
		if (Tag->type == MULTIBOOT_TAG_TYPE_END)
			break;

		switch (Tag->type)
		{
		case MULTIBOOT_TAG_TYPE_CMDLINE:
		{
			multiboot_tag_string *cmdline = (multiboot_tag_string *)Tag;
			strncpy(mb2binfo.Kernel.CommandLine, cmdline->string, strlen(cmdline->string));
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
		{
			multiboot_tag_string *blName = (multiboot_tag_string *)Tag;
			strncpy(mb2binfo.Bootloader.Name, blName->string, strlen(blName->string));
			break;
		}
		case MULTIBOOT_TAG_TYPE_MODULE:
		{
			static int module_count = 0;
			if (module_count >= MAX_MODULES)
				break;

			multiboot_tag_module *module = (multiboot_tag_module *)Tag;

			mb2binfo.Modules[module_count].Address = (void *)(uintptr_t)module->mod_start;
			mb2binfo.Modules[module_count].Size = (size_t)(module->mod_end - module->mod_start);

			strncpy(mb2binfo.Modules[module_count].Path, "(null)", 6);
			mb2binfo.Modules[module_count].CommandLine[0] = '\0';

			if (module->cmdline[0] != '\0')
			{
				int len = strlen(module->cmdline);
				if (len > 255)
					len = 255;
				strncpy(mb2binfo.Modules[module_count].CommandLine, module->cmdline, len);
			}

			module_count++;
			break;
		}
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
		{
			// multiboot_tag_basic_meminfo *meminfo = (multiboot_tag_basic_meminfo *)Tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOTDEV:
		{
			// multiboot_tag_bootdev *bootdev = (multiboot_tag_bootdev *)Tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_MMAP:
		{
			multiboot_tag_mmap *mmap = (multiboot_tag_mmap *)Tag;
			size_t EntryCount = mmap->size / sizeof(multiboot_mmap_entry);
			mb2binfo.Memory.Entries = EntryCount;
			for (uint32_t i = 0; i < EntryCount; i++)
			{
				if (i > MAX_MEMORY_ENTRIES)
					break;

				multiboot_mmap_entry entry = mmap->entries[i];
				mb2binfo.Memory.Size += entry.len;
				switch (entry.type)
				{
				case MULTIBOOT_MEMORY_AVAILABLE:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = Usable;
					break;
				case MULTIBOOT_MEMORY_RESERVED:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = Reserved;
					break;
				case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = ACPIReclaimable;
					break;
				case MULTIBOOT_MEMORY_NVS:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = ACPINVS;
					break;
				case MULTIBOOT_MEMORY_BADRAM:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = BadMemory;
					break;
				default:
					mb2binfo.Memory.Entry[i].BaseAddress = (void *)entry.addr;
					mb2binfo.Memory.Entry[i].Length = entry.len;
					mb2binfo.Memory.Entry[i].Type = Unknown;
					break;
				}
			}
			break;
		}
		case MULTIBOOT_TAG_TYPE_VBE:
		{
			// multiboot_tag_vbe *vbe = (multiboot_tag_vbe *)Tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
		{
			multiboot_tag_framebuffer *fb = (multiboot_tag_framebuffer *)Tag;
			static int fbCount = 0;
			mb2binfo.Framebuffer[fbCount].BaseAddress = (void *)fb->common.framebuffer_addr;
			mb2binfo.Framebuffer[fbCount].Width = fb->common.framebuffer_width;
			mb2binfo.Framebuffer[fbCount].Height = fb->common.framebuffer_height;
			mb2binfo.Framebuffer[fbCount].Pitch = fb->common.framebuffer_pitch;
			mb2binfo.Framebuffer[fbCount].BitsPerPixel = fb->common.framebuffer_bpp;
			switch (fb->common.framebuffer_type)
			{
			case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
				mb2binfo.Framebuffer[fbCount].Type = Indexed;
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
				mb2binfo.Framebuffer[fbCount].Type = RGB;
				mb2binfo.Framebuffer[fbCount].RedMaskSize = fb->framebuffer_red_mask_size;
				mb2binfo.Framebuffer[fbCount].RedMaskShift = fb->framebuffer_red_field_position;
				mb2binfo.Framebuffer[fbCount].GreenMaskSize = fb->framebuffer_green_mask_size;
				mb2binfo.Framebuffer[fbCount].GreenMaskShift = fb->framebuffer_green_field_position;
				mb2binfo.Framebuffer[fbCount].BlueMaskSize = fb->framebuffer_blue_mask_size;
				mb2binfo.Framebuffer[fbCount].BlueMaskShift = fb->framebuffer_blue_field_position;
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
				mb2binfo.Framebuffer[fbCount].Type = EGA;
				break;
			default:
				mb2binfo.Framebuffer[fbCount].Type = Unknown_Framebuffer_Type;
				break;
			}
			fbCount++;
			break;
		}
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
		{
			multiboot_tag_elf_sections *elf = (multiboot_tag_elf_sections *)Tag;
			mb2binfo.Kernel.Symbols.Num = elf->num;
			mb2binfo.Kernel.Symbols.EntSize = elf->entsize;
			mb2binfo.Kernel.Symbols.Shndx = elf->shndx;
			mb2binfo.Kernel.Symbols.Sections = reinterpret_cast<uintptr_t>(elf->sections);
			break;
		}
		case MULTIBOOT_TAG_TYPE_APM:
		{
			// multiboot_tag_apm *apm = (multiboot_tag_apm *)Tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI32:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.ST = 1;

			multiboot_tag_efi32 *efi32 = (multiboot_tag_efi32 *)Tag;
			mb2binfo.EFI.SystemTable = (void *)(uintptr_t)efi32->pointer;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI64:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.ST = 1;

			multiboot_tag_efi64 *efi64 = (multiboot_tag_efi64 *)Tag;
			mb2binfo.EFI.SystemTable = (void *)(uintptr_t)efi64->pointer;
			break;
		}
		case MULTIBOOT_TAG_TYPE_SMBIOS:
		{
			multiboot_tag_smbios *smbios = (multiboot_tag_smbios *)Tag;
			mb2binfo.SMBIOSPtr = (void *)smbios->tables;
			break;
		}
		case MULTIBOOT_TAG_TYPE_ACPI_OLD:
		{
			mb2binfo.RSDP = (BootInfo::RSDPInfo *)((multiboot_tag_old_acpi *)Tag)->rsdp;
			break;
		}
		case MULTIBOOT_TAG_TYPE_ACPI_NEW:
		{
			mb2binfo.RSDP = (BootInfo::RSDPInfo *)((multiboot_tag_new_acpi *)Tag)->rsdp;
			break;
		}
		case MULTIBOOT_TAG_TYPE_NETWORK:
		{
			// multiboot_tag_network *net = (multiboot_tag_network *)Tag;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI_MMAP:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.MemoryMap = 1;

			multiboot_tag_efi_mmap *efi_mmap = (multiboot_tag_efi_mmap *)Tag;
			mb2binfo.EFI.MemoryMap.BaseAddress = (void *)efi_mmap->efi_mmap;
			mb2binfo.EFI.MemoryMap.DescriptorSize = efi_mmap->descr_size;
			mb2binfo.EFI.MemoryMap.DescriptorVersion = efi_mmap->descr_vers;
			mb2binfo.EFI.MemoryMap.NumberOfEntries = (efi_mmap->size - sizeof(multiboot_tag_efi_mmap)) / efi_mmap->descr_size;
			// mb2binfo.EFI.MemoryMap.NumberOfEntries = efi_mmap->size / efi_mmap->descr_size;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI_BS:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.BS = 1;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI32_IH:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.IH = 1;

			multiboot_tag_efi32_ih *efi32_ih = (multiboot_tag_efi32_ih *)Tag;
			mb2binfo.EFI.ImageHandle = (void *)(uintptr_t)efi32_ih->pointer;
			break;
		}
		case MULTIBOOT_TAG_TYPE_EFI64_IH:
		{
			mb2binfo.EFI.Info.Enabled = 1;
			mb2binfo.EFI.Info.IH = 1;

			multiboot_tag_efi64_ih *efi64_ih = (multiboot_tag_efi64_ih *)Tag;
			mb2binfo.EFI.ImageHandle = (void *)(uintptr_t)efi64_ih->pointer;
			break;
		}
		case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
		{
			multiboot_tag_load_base_addr *load_base_addr = (multiboot_tag_load_base_addr *)Tag;
			mb2binfo.Kernel.PhysicalBase = (void *)(uint64_t)load_base_addr->load_base_addr;
			mb2binfo.Kernel.VirtualBase = (void *)(uint64_t)(load_base_addr->load_base_addr + 0xFFFFFFFF80000000);
			mb2binfo.Kernel.Size = ((uint64_t)&_kernel_end - (uint64_t)&_kernel_start) + ((uint64_t)&_bootstrap_end - (uint64_t)&_bootstrap_start);
			break;
		}
		default:
			break;
		}
	}

	KernelEntry(&mb2binfo);
}
