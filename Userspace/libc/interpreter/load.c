/*
	This file is part of Fennix C Library.

	Fennix C Library is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix C Library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix C Library. If not, see <https://www.gnu.org/licenses/>.
*/

#include <bits/libc.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <stddef.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "elf.h"
#include "misc.h"

typedef struct ElfInfo
{
	Elf_Ehdr Header;
	Elf_Phdr DynamicHeader;
	uintptr_t BaseAddress;
	Elf_Dyn *Dynamic;
	char *Path;

	struct
	{
		Elf_Addr PLTGOT, HASH, STRTAB, SYMTAB, RELA, REL, TEXTREL, JMPREL;
		Elf_Addr BIND_NOW, INIT, FINI, RPATH, SYMBOLIC, INIT_ARRAY;
		Elf_Addr FINI_ARRAY, PREINIT_ARRAY, RUNPATH, FLAGS;
	} DynamicTable;

	struct
	{
#ifdef __LP64__
		_Static_assert(sizeof(Elf64_Xword) == sizeof(size_t), "Elf64_Xword and size_t are not the same size");
#else
		_Static_assert(sizeof(Elf32_Word) == sizeof(size_t), "Elf32_Word and size_t are not the same size");
#endif
		size_t PLTRELSZ, RELASZ, RELAENT, STRSZ, SYMENT, RELSZ, RELENT, PLTREL;
		size_t INIT_ARRAYSZ, FINI_ARRAYSZ, PREINIT_ARRAYSZ;
	} DynamicSize;

	union
	{
		struct
		{
			uint8_t Relocated : 1;
			uint8_t IsLibrary : 1;
			uint8_t __padding : 6;
		};
		uint8_t raw;
	} Flags;

	struct ElfInfo *prev;
	struct ElfInfo *next;
} ElfInfo;

ElfInfo *elf_list_head = NULL;

ElfInfo *AllocateLib()
{
	ElfInfo *new_node = mini_malloc(sizeof(ElfInfo));
	if (!new_node)
	{
		printf("Failed to allocate memory for new library\n");
		return NULL;
	}

	memset(new_node, 0, sizeof(ElfInfo));

	if (!elf_list_head)
	{
		elf_list_head = new_node;
		return new_node;
	}

	ElfInfo *current = elf_list_head;
	while (current->next)
		current = current->next;

	current->next = new_node;
	new_node->prev = current;
	return new_node;
}

void FreeLib(ElfInfo *node)
{
	if (!node)
		return;

	if (node->prev)
		node->prev->next = node->next;
	else
		elf_list_head = node->next;

	if (node->next)
		node->next->prev = node->prev;

	mini_free(node);
}

ElfInfo *SearchLib(char *Path)
{
	ElfInfo *current = elf_list_head;
	while (current)
	{
		if (strcmp(current->Path, Path) == 0)
			return current;
		current = current->next;
	}
	return NULL;
}

__attribute__((naked, used, no_stack_protector)) void _dl_runtime_resolve()
{
#if defined(__amd64__)
	__asm__(
		"pop %r11\n" /* Pop lazy resolve arguments */
		"pop %r10\n"

		"push %rdi\n"
		"push %rsi\n"
		"push %rdx\n"
		"push %rcx\n"
		"push %r8\n"
		"push %r9\n"

		"mov %r11, %rdi\n" /* Move the first argument to rdi */
		"mov %r10, %rsi\n" /* Move the second argument to rsi (rel index) */
		"call _dl_fixup\n" /* Call _dl_fixup */
		"mov %rax, %r11\n" /* Move the return value to r11 */

		"pop %r9\n"
		"pop %r8\n"
		"pop %rcx\n"
		"pop %rdx\n"
		"pop %rsi\n"
		"pop %rdi\n"

		"jmp *%r11\n"); /* Jump to the return value */
#elif defined(__i386__)
#warning "i386 _dl_runtime_resolve not implemented"
#elif defined(__aarch64__)
#warning "aarch64 not implemented"
#endif
}

int RelocateHelper(ElfInfo *Info, Elf_Rela *Rela, short IsRel, void **Relocated);
__attribute__((noinline)) void *_dl_fixup(ElfInfo *Info, long RelIndex)
{
	void *ret = NULL;
	if (Info->DynamicSize.PLTREL == DT_REL)
		RelocateHelper(Info, (Elf_Rela *)(Info->DynamicTable.JMPREL + RelIndex), 1, &ret);
	else if (Info->DynamicSize.PLTREL == DT_RELA)
		RelocateHelper(Info, &((Elf_Rela *)Info->DynamicTable.JMPREL)[RelIndex], 0, &ret);
	return ret;
}

#ifdef __fennix__
#include <fennix/syscalls.h>
#endif

int _dl_preload()
{
#ifdef __fennix__
	call_api_version(0);
#endif

	/* TODO: Do aditional checks for miscellaneous things */

	/* Everything is ok, continue. */
	return 0;
}

void HandleGOT(ElfInfo *Info, Elf_Addr Offset)
{
	Elf_Addr *got = (Elf_Addr *)Offset;
	got[0] = (Elf_Addr)Info->Dynamic;
	got[1] = (Elf_Addr)Info;
	got[2] = (Elf_Addr)&_dl_runtime_resolve;
}

void AdjustDynamicTable(Elf_Dyn *elem, ElfInfo *Info)
{
	switch (elem->d_tag)
	{
	case DT_PLTGOT:
	case DT_HASH:
	case DT_STRTAB:
	case DT_SYMTAB:
	case DT_RELA:
	case DT_INIT:
	case DT_FINI:
	case DT_REL:
	case DT_JMPREL:
	case DT_INIT_ARRAY:
	case DT_FINI_ARRAY:
		elem->d_un.d_ptr += Info->BaseAddress;
		break;

	default:
		break;
	}
}

void CreateInfoTables(Elf_Dyn *elem, ElfInfo *Info)
{
	switch (elem->d_tag)
	{
	case DT_NEEDED:
		break;
	case DT_PLTRELSZ:
		Info->DynamicSize.PLTRELSZ = elem->d_un.d_val;
		break;
	case DT_PLTGOT:
		HandleGOT(Info, elem->d_un.d_ptr);
		Info->DynamicTable.PLTGOT = elem->d_un.d_ptr;
		break;
	case DT_HASH:
		Info->DynamicTable.HASH = elem->d_un.d_ptr;
		break;
	case DT_STRTAB:
		Info->DynamicTable.STRTAB = elem->d_un.d_ptr;
		break;
	case DT_SYMTAB:
		Info->DynamicTable.SYMTAB = elem->d_un.d_ptr;
		break;
	case DT_RELA:
		Info->DynamicTable.RELA = elem->d_un.d_ptr;
		break;
	case DT_RELASZ:
		Info->DynamicSize.RELASZ = elem->d_un.d_val;
		break;
	case DT_RELAENT:
		Info->DynamicSize.RELAENT = elem->d_un.d_val;
		break;
	case DT_STRSZ:
		Info->DynamicSize.STRSZ = elem->d_un.d_val;
		break;
	case DT_SYMENT:
		Info->DynamicSize.SYMENT = elem->d_un.d_val;
		break;
	case DT_INIT:
		Info->DynamicTable.INIT = elem->d_un.d_ptr;
		break;
	case DT_FINI:
		Info->DynamicTable.FINI = elem->d_un.d_ptr;
		break;
	case DT_RPATH:
		Info->DynamicTable.RPATH = elem->d_un.d_ptr;
		break;
	case DT_SYMBOLIC:
		Info->DynamicTable.SYMBOLIC = elem->d_un.d_ptr;
		break;
	case DT_REL:
		Info->DynamicTable.REL = elem->d_un.d_ptr;
		break;
	case DT_RELSZ:
		Info->DynamicSize.RELSZ = elem->d_un.d_val;
		break;
	case DT_RELENT:
		Info->DynamicSize.RELENT = elem->d_un.d_val;
		break;
	case DT_PLTREL:
		Info->DynamicSize.PLTREL = elem->d_un.d_val;
		break;
	// case DT_DEBUG:
	case DT_TEXTREL:
		Info->DynamicTable.TEXTREL = elem->d_un.d_ptr;
		break;
	case DT_JMPREL:
		Info->DynamicTable.JMPREL = elem->d_un.d_ptr;
		break;
	case DT_BIND_NOW:
		Info->DynamicTable.BIND_NOW = elem->d_un.d_ptr;
		break;
	case DT_INIT_ARRAY:
		Info->DynamicTable.INIT_ARRAY = elem->d_un.d_ptr;
		break;
	case DT_FINI_ARRAY:
		Info->DynamicTable.FINI_ARRAY = elem->d_un.d_ptr;
		break;
	case DT_INIT_ARRAYSZ:
		Info->DynamicSize.INIT_ARRAYSZ = elem->d_un.d_val;
		break;
	case DT_FINI_ARRAYSZ:
		Info->DynamicSize.FINI_ARRAYSZ = elem->d_un.d_val;
		break;
	case DT_RUNPATH:
		Info->DynamicTable.RUNPATH = elem->d_un.d_ptr;
		break;
	case DT_FLAGS:
		Info->DynamicTable.FLAGS = elem->d_un.d_ptr;
		break;
	// case DT_ENCODING:
	case DT_PREINIT_ARRAY:
		Info->DynamicTable.PREINIT_ARRAY = elem->d_un.d_ptr;
		break;
	case DT_PREINIT_ARRAYSZ:
		Info->DynamicSize.PREINIT_ARRAYSZ = elem->d_un.d_val;
		break;
	case DT_LOOS:
	case DT_SUNW_RTLDINF:
	case DT_HIOS:
	case DT_VALRNGLO:
	case DT_CHECKSUM:
	case DT_PLTPADSZ:
	case DT_MOVEENT:
	case DT_MOVESZ:
	case DT_FEATURE_1:
	case DT_POSFLAG_1:
	case DT_SYMINSZ:
	case DT_SYMINENT:
	// case DT_VALRNGHI:
	case DT_ADDRRNGLO:
	case DT_CONFIG:
	case DT_DEPAUDIT:
	case DT_AUDIT:
	case DT_PLTPAD:
	case DT_MOVETAB:
	case DT_SYMINFO:
	// case DT_ADDRRNGHI:
	case DT_RELACOUNT:
	case DT_RELCOUNT:
	case DT_FLAGS_1:
	case DT_VERDEF:
	case DT_VERDEFNUM:
	case DT_VERNEED:
	case DT_VERNEEDNUM:
	case DT_LOPROC:
	case DT_SPARC_REGISTER:
	case DT_AUXILIARY:
	case DT_USED:
	case DT_FILTER:
	// case DT_HIPROC:
	default:
		break;
	}
}

int LoadElf(int, char *, ElfInfo **);
void ProcessNeededLibraries(Elf_Dyn *elem, ElfInfo *Info)
{
	char *libPath = (char *)Info->DynamicTable.STRTAB + elem->d_un.d_val;
	ElfInfo *info = NULL;

	char fullLibPath[PATH_MAX];
	int found = 0;

	char *ldLibPath = getenv("LD_LIBRARY_PATH");
	if (ldLibPath)
	{
		char *pathCopy = strdup(ldLibPath);
		char *path = strtok(pathCopy, ":");

		while (path)
		{
			strcpy(fullLibPath, path);
			if (fullLibPath[strlen(fullLibPath) - 1] != '/')
				strcat(fullLibPath, "/");
			strcat(fullLibPath, libPath);

			if (sysdep(Access)(fullLibPath, F_OK) == 0)
			{
				found = 1;
				break;
			}

			path = strtok(NULL, ":");
		}

		mini_free(pathCopy);
		if (found)
			goto load_lib;
	}

	const char *standardPaths[] = {
		"/lib/",
		"/usr/lib/",
		"/lib/",
		"/usr/local/lib/",
		"/usr/local/lib64/",
		"/usr/lib64/",
		"/lib64/"};

	for (size_t i = 0; i < sizeof(standardPaths) / sizeof(standardPaths[0]); i++)
	{
		strcpy(fullLibPath, standardPaths[i]);
		strcat(fullLibPath, libPath);

		if (sysdep(Access)(fullLibPath, F_OK) == 0)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		printf("dl: Library %s not found in search paths\n", libPath);
		return;
	}

load_lib:
	int fd = sysdep(Open)(fullLibPath, O_RDONLY, 0644);
	int status = LoadElf(fd, fullLibPath, &info);
	elem->d_un.d_ptr = (uintptr_t)info; /* if LoadElf fails, info will still be NULL */
	sysdep(Close)(fd);
	if (status < 0) /* announce that LoadElf failed */
		printf("dl: Can't load %s\n", fullLibPath);
}

void ProcessDynamicTable(ElfInfo *Info)
{
	for (size_t i = 0;; i++)
	{
		Elf_Dyn *elem = &Info->Dynamic[i];
		if (elem->d_tag == DT_NULL)
			break;
		AdjustDynamicTable(elem, Info);
		CreateInfoTables(elem, Info);
	}

	/* TODO: Optimize this, we don't have to recheck every element */
	for (size_t i = 0;; i++)
	{
		Elf_Dyn *elem = &Info->Dynamic[i];
		if (elem->d_tag == DT_NULL)
			break;
		if (elem->d_tag != DT_NEEDED)
			continue;
		ProcessNeededLibraries(elem, Info);
	}
}

uintptr_t GetASLR()
{
#ifdef DEBUG
	static uintptr_t __aslr_stub_next = 0;
	__aslr_stub_next += 0x1000000;
	return __aslr_stub_next;
#else
	/* FIXME: implement real ASLR */
	static uintptr_t __aslr_stub_next = 0;
	__aslr_stub_next += 0x1000000;
	return __aslr_stub_next;
#endif
}

int LoadElfPhdrEXEC(int fd, ElfInfo *Info)
{
	printf("dl: ET_EXEC not implemented yet\n");
	return -ENOSYS;
}

uintptr_t DynamicCreateBase(int fd, ElfInfo *Info)
{
	uintptr_t base = GetASLR();
	if ((uintptr_t)base <= 0)
	{
		printf("dl: Can't get ASLR\n");
		_Exit(-1);
	}
	Info->BaseAddress = base;
	return base;
}

int LoadElfPhdrDYN(int fd, ElfInfo *Info)
{
	Elf_Ehdr header = Info->Header;
	Info->Dynamic = NULL;
	uintptr_t base = 0;
	Elf_Phdr phdr, lastLOAD;

	for (Elf_Half i = 0; i < header.e_phnum; i++)
	{
		ssize_t read = sysdep(PRead)(fd, &phdr, sizeof(Elf_Phdr), header.e_phoff + (header.e_phentsize * i));
		if (read != sizeof(Elf_Phdr))
		{
			printf("dl: Can't read program header %d\n", i);
			return (int)read;
		}

		switch (phdr.p_type)
		{
		case PT_LOAD:
		{
			if (phdr.p_memsz == 0)
				continue;

			if (base == 0)
				base = DynamicCreateBase(fd, Info);

			int mmapProt = 0;
			if (phdr.p_flags & PF_X)
				mmapProt |= PROT_EXEC;
			if (phdr.p_flags & PF_W)
				mmapProt |= PROT_WRITE;
			if (phdr.p_flags & PF_R)
				mmapProt |= PROT_READ;

			off_t sectionOffset = ALIGN_DOWN(phdr.p_vaddr, phdr.p_align);
			size_t sectionSize = ALIGN_UP(phdr.p_memsz + (phdr.p_vaddr - sectionOffset), phdr.p_align);
			uintptr_t section = (uintptr_t)sysdep(MemoryMap)((void *)(base + sectionOffset),
															 sectionSize, mmapProt,
															 MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
															 -1, 0);
			sectionOffset = phdr.p_vaddr - ALIGN_DOWN(phdr.p_vaddr, phdr.p_align);

			if (phdr.p_filesz > 0)
			{
				ssize_t read = sysdep(PRead)(fd, (void *)(section + sectionOffset), phdr.p_filesz, phdr.p_offset);
				if (read != phdr.p_filesz)
				{
					printf("dl: Can't read segment %d in PT_LOAD\n", i);
					return (int)read;
				}
			}

			if (phdr.p_memsz - phdr.p_filesz > 0)
			{
				/* TODO: Do we really have to do this? Kernel already zeros the memory for us */
				void *zero = (void *)(section + sectionOffset + phdr.p_filesz);
				memset(zero, 0, phdr.p_memsz - phdr.p_filesz);
			}
			lastLOAD = phdr;
			break;
		}
		case PT_DYNAMIC:
		{
			Elf_Dyn *dynamicTable = NULL;
			if (phdr.p_vaddr == lastLOAD.p_vaddr && phdr.p_memsz < lastLOAD.p_memsz)
			{
				/* The dynamic section is inside the last LOAD segment */
				dynamicTable = (Elf_Dyn *)(base + phdr.p_vaddr);
			}
			else
			{
				int mmapProt = 0;
				if (phdr.p_flags & PF_X)
					mmapProt |= PROT_EXEC;
				if (phdr.p_flags & PF_W)
					mmapProt |= PROT_WRITE;
				if (phdr.p_flags & PF_R)
					mmapProt |= PROT_READ;

				dynamicTable = (Elf_Dyn *)sysdep(MemoryMap)(0, ALIGN_UP(phdr.p_memsz, phdr.p_align),
															mmapProt, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
															-1, 0);

				if ((intptr_t)dynamicTable <= 0)
				{
					printf("dl: Can't allocate memory for PT_DYNAMIC\n");
					return (int)(uintptr_t)dynamicTable;
				}

				read = sysdep(PRead)(fd, dynamicTable, phdr.p_memsz, phdr.p_offset);
				if (read != phdr.p_memsz)
				{
					printf("dl: Can't read PT_DYNAMIC\n");
					return (int)read;
				}
			}

			Info->Dynamic = dynamicTable;
			Info->DynamicHeader = phdr;
			break;
		}
		case PT_INTERP:
			break;
		case PT_NOTE:
			break;
		case PT_SHLIB:
			break;
		case PT_PHDR:
			break;
		case PT_TLS:
		{
			printf("dl: PT_TLS not implemented yet\n");
			break;
		}
		default:
		{
			printf("dl: Unimplemented program header type %d\n", phdr.p_type);
			break;
		}
		}
	}

	return 0;
}

int CheckElfEhdr(Elf_Ehdr *ehdr, char *Path)
{
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
		ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
		ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
		ehdr->e_ident[EI_MAG3] != ELFMAG3)
	{
		printf("dl: %s is not an ELF file\n", Path);
		return -EINVAL;
	}

#ifdef __LP64__
	const int elfClass = ELFCLASS64;
#else
	const int elfClass = ELFCLASS32;
#endif

	if (ehdr->e_ident[EI_CLASS] != elfClass)
	{
		printf("dl: %s is not a %s-bit ELF file\n",
			   Path, elfClass == ELFCLASS64 ? "64" : "32");
		return -EINVAL;
	}

	/* TODO: check LSB MSB */

	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT || ehdr->e_version != EV_CURRENT)
	{
		printf("dl: %s has an unsupported ELF version\n", Path);
		return -EINVAL;
	}

	if (ehdr->e_type != ET_DYN && ehdr->e_type != ET_EXEC)
	{
		printf("dl: %s is not a shared object or executable\n", Path);
		return -EINVAL;
	}

	return 0;
}

int LoadElf(int fd, char *Path, ElfInfo **Out)
{
	ElfInfo *info = SearchLib(Path);
	if (info != NULL)
	{
		*Out = info;
		return 0;
	}

	Elf_Ehdr header;
	sysdep(PRead)(fd, &header, sizeof(Elf_Ehdr), 0);

	int status = CheckElfEhdr(&header, Path);
	if (status != 0)
		return status;

	info = AllocateLib();
	info->Header = header;
	info->Path = (char *)sysdep(MemoryMap)(0,
										   ALIGN_UP(strlen(Path) + 1, 0x1000 /* TODO: get page size from kernel */),
										   PROT_READ,
										   MAP_ANONYMOUS | MAP_PRIVATE,
										   -1, 0);
	if ((intptr_t)info->Path <= 0)
	{
		printf("dl: Can't allocate memory for path\n");
		FreeLib(info);
		return (int)(uintptr_t)info->Path;
	}

	memcpy(info->Path, Path, strlen(Path) + 1);

	switch (header.e_type)
	{
	case ET_REL:
		printf("dl: ET_REL not implemented yet\n");
		status = -ENOSYS;
		break;
	case ET_EXEC:
		status = LoadElfPhdrEXEC(fd, info);
		break;
	case ET_DYN:
		status = LoadElfPhdrDYN(fd, info);
		break;
	case ET_CORE:
		printf("dl: ET_CORE not implemented yet\n");
		status = -ENOSYS;
		break;
	case ET_NONE:
		printf("dl: ET_NONE???\n");
		status = -EINVAL;
		break;
	default:
		printf("dl: Unsupported ELF type %d\n", header.e_type);
		status = -EINVAL;
		break;
	}

	if (status < 0)
	{
		sysdep(MemoryUnmap)((void *)info->Path, ALIGN_UP(strlen(Path) + 1, 0x1000));
		FreeLib(info);
		return status;
	}

	ProcessDynamicTable(info);
	*Out = info;
	return 0;
}

uintptr_t GetSymbolAddress(ElfInfo *Info, const char *SymbolName)
{
	Elf64_Sym *sym = find_symbol(SymbolName,
								 (uint32_t *)Info->DynamicTable.HASH,
								 (Elf64_Sym *)Info->DynamicTable.SYMTAB,
								 (const char *)Info->DynamicTable.STRTAB);
	if (sym == NULL)
		return (-ENOSYS);
	return Info->BaseAddress + sym->st_value;
}

int ResolveExternalSymbol(ElfInfo *Info, uintptr_t *symAddress, Elf_Sym *sym, const char *symName)
{
	*symAddress = (-ENOSYS);

	for (size_t i = 0; *symAddress == (-ENOSYS); i++)
	{
		Elf_Dyn *dyn = &Info->Dynamic[i];
		if (dyn->d_tag == DT_NULL)
			break;

		if (dyn->d_tag != DT_NEEDED)
			continue;

		ElfInfo *lib = (ElfInfo *)dyn->d_un.d_ptr;
		*symAddress = GetSymbolAddress(lib, symName);
	}

	if (*symAddress != (-ENOSYS))
		return 0;

	printf("%s: Unresolved symbol: %s\n", Info->Path, symName);
	if (ELF_ST_BIND(sym->st_info) != STB_WEAK)
		return -EINVAL;
	*symAddress = 0;
	return 0;
}

int ApplyRelocation(ElfInfo *Info, uintptr_t *reloc, Elf_Rela *Rela, size_t reloSize)
{
	switch (reloSize)
	{
	case 0:
		break;
	case sizeof(uint8_t):
		*reloc += *(uint8_t *)(Info->BaseAddress + Rela->r_offset);
		break;
	case sizeof(uint16_t):
		*reloc += *(uint16_t *)(Info->BaseAddress + Rela->r_offset);
		break;
	case sizeof(uint32_t):
		*reloc += *(uint32_t *)(Info->BaseAddress + Rela->r_offset);
		break;
	case sizeof(uint64_t):
		*reloc += *(uint64_t *)(Info->BaseAddress + Rela->r_offset);
		break;
	default:
	{
		printf("dl: Unsupported size for relocation\n");
		return -EINVAL;
	}
	}
	return 0;
}

int CalculateRelocation(ElfInfo *Info, uintptr_t *reloc, Elf_Rela *Rela, size_t reloSize)
{
	switch (reloSize)
	{
	case 0:
		break;
	case sizeof(uint8_t):
		*(uint8_t *)(Info->BaseAddress + Rela->r_offset) = *reloc;
		break;
	case sizeof(uint16_t):
		*(uint16_t *)(Info->BaseAddress + Rela->r_offset) = *reloc;
		break;
	case sizeof(uint32_t):
		*(uint32_t *)(Info->BaseAddress + Rela->r_offset) = *reloc;
		break;
	case sizeof(uint64_t):
		*(uint64_t *)(Info->BaseAddress + Rela->r_offset) = *reloc;
		break;
	default:
	{
		printf("dl: Unsupported size for relocation\n");
		return -EINVAL;
	}
	}
	return 0;
}

int RelocateHelper(ElfInfo *Info, Elf_Rela *Rela, short IsRel, void **Relocated)
{
	uintptr_t reloc = 0;
	uintptr_t symAddress = 0;
	size_t symSize = 0;

	uint32_t symIndex = ELF_R_SYM(Rela->r_info);
	if (symIndex)
	{
		Elf_Sym *sym = (Elf_Sym *)(Info->DynamicTable.SYMTAB + symIndex * Info->DynamicSize.SYMENT);
		const char *symName = (const char *)(Info->DynamicTable.STRTAB + sym->st_name);
		symSize = sym->st_size;

		if (!(ELF_R_TYPE(Rela->r_info) == R_COPY) && sym->st_shndx)
			symAddress = Info->BaseAddress + sym->st_value;
		else if (ResolveExternalSymbol(Info, &symAddress, sym, symName) < 0)
			return -EINVAL;
	}

	size_t reloSize = 0;
	int addAddend = 0;

	enum RelocationTypes relType = ELF_R_TYPE(Rela->r_info);
	switch (relType)
	{
	case R_NONE:
		break;
	case R_X86_64_64:
	{
		reloSize = 8;
		reloc = symAddress;
		addAddend = 1;
		break;
	}
	case R_COPY:
	{
		if (symAddress == 0)
		{
			printf("dl: Copy undefined weak symbol %d\n", ELF_R_SYM(Rela->r_info));
			return -EINVAL;
		}
		memcpy((void *)(Info->BaseAddress + Rela->r_offset),
			   (void *)symAddress,
			   symSize);
		break;
	}
	case R_GLOB_DAT:
	case R_JMP_SLOT:
	{
		reloSize = 8;
		reloc = symAddress;
		break;
	}
	case R_RELATIVE:
	{
		reloSize = 8;
		reloc = Info->BaseAddress;
		addAddend = 1;
		break;
	}
#if __LP64__
	case R_DTPMOD64:
	{
		printf("dl: i don't know what to do with DTPMOD64\n");
		reloc = Info->BaseAddress;
		break;
	}
#if defined(__amd64__)
	case R_DTPOFF64:
	{
		printf("dl: i don't know what to do with DTPOFF64\n");
		reloc = symAddress + Rela->r_addend;
		break;
	}
	case R_TPOFF64:
	{
		printf("dl: i don't know what to do with TPOFF64\n");
		reloc = symAddress + Rela->r_addend;
		break;
	}
#endif
#endif // __LP64__
	default:
	{
		printf("dl: Unsupported relocation type %d\n", relType);
		return -EINVAL;
	}
	}

	if (addAddend)
	{
		if (IsRel == 0)
			reloc += Rela->r_addend;
		else if (ApplyRelocation(Info, &reloc, Rela, reloSize) < 0)
			return -EINVAL;
	}

	CalculateRelocation(Info, &reloc, Rela, reloSize);
	if (Relocated != NULL)
		*Relocated = (void *)reloc;
	return 0;
}

int HandleRelocations(ElfInfo *Info);
void SearchNeeded(ElfInfo *Info)
{
	for (size_t i = 0;; i++)
	{
		Elf_Dyn *elem = &Info->Dynamic[i];
		if (elem->d_tag == DT_NULL)
			break;

		if (elem->d_tag != DT_NEEDED)
			continue;

		HandleRelocations((ElfInfo *)elem->d_un.d_ptr);
	}
}

int HandleRelocations(ElfInfo *Info)
{
	if (Info->Flags.Relocated)
		return 0;
	SearchNeeded(Info);

	if (Info->DynamicTable.REL != ((Elf_Addr)0) && Info->DynamicSize.RELENT != 0)
	{
		for (size_t i = 0; i < Info->DynamicSize.RELSZ / Info->DynamicSize.RELENT; i++)
			RelocateHelper(Info, (Elf_Rela *)(Info->DynamicTable.REL + i * Info->DynamicSize.RELENT), 1, NULL);
	}

	if (Info->DynamicTable.RELA != ((Elf_Addr)0) && Info->DynamicSize.RELAENT != 0)
	{
		for (size_t i = 0; i < Info->DynamicSize.RELASZ / Info->DynamicSize.RELAENT; i++)
			RelocateHelper(Info, (Elf_Rela *)(Info->DynamicTable.RELA + i * Info->DynamicSize.RELAENT), 0, NULL);
	}

	if (Info->DynamicTable.JMPREL == ((Elf_Addr)0) || Info->DynamicSize.PLTRELSZ == 0)
		return 0;

	if (Info->DynamicSize.PLTREL != DT_REL && Info->DynamicSize.PLTREL != DT_RELA)
	{
		printf("dl: Wrong PLT relocation type %d\n", Info->DynamicSize.PLTREL);
		return -EINVAL;
	}

	if (Info->DynamicTable.BIND_NOW != ((Elf_Addr)0))
	{
		if (Info->DynamicSize.PLTREL == DT_REL)
		{
			for (size_t i = 0; i < Info->DynamicSize.PLTRELSZ / sizeof(Elf_Rel); i++)
				RelocateHelper(Info, (Elf_Rela *)&((Elf_Rel *)Info->DynamicTable.JMPREL)[i], 1, NULL);
		}
		else if (Info->DynamicSize.PLTREL == DT_RELA)
		{
			for (size_t i = 0; i < Info->DynamicSize.PLTRELSZ / sizeof(Elf_Rela); i++)
				RelocateHelper(Info, (Elf_Rela *)&((Elf_Rela *)Info->DynamicTable.JMPREL)[i], 0, NULL);
		}

		Info->Flags.Relocated = 1;
		return 0;
	}

	size_t relsize = Info->DynamicSize.PLTREL == DT_REL ? sizeof(Elf_Rel) : sizeof(Elf_Rela);
	for (size_t i = 0; i < Info->DynamicSize.PLTRELSZ / relsize; i++)
	{
		Elf64_Xword info = (Info->DynamicSize.PLTREL == DT_REL
								? ((Elf_Rel *)Info->DynamicTable.JMPREL)[i].r_info
								: ((Elf_Rela *)Info->DynamicTable.JMPREL)[i].r_info);

		Elf_Addr offset = (Info->DynamicSize.PLTREL == DT_REL
							   ? ((Elf_Rel *)Info->DynamicTable.JMPREL)[i].r_offset
							   : ((Elf_Rela *)Info->DynamicTable.JMPREL)[i].r_offset);

		if (ELF_R_TYPE(info) != R_JMP_SLOT)
		{
			printf("dl: Wrong JMPREL type %d\n", ELF_R_TYPE(info));
			return -EINVAL;
		}

		/* FIXME: HANDLE THIS RIGHT */
		if (Info->DynamicSize.PLTREL == DT_REL)
		{
			Elf_Addr *slot = (Elf_Addr *)(Info->BaseAddress + offset);
			*slot += Info->BaseAddress;
			if (*slot == Info->BaseAddress)
				RelocateHelper(Info, (Elf_Rela *)&((Elf_Rel *)Info->DynamicTable.JMPREL)[i], 1, NULL);
		}
		else if (Info->DynamicSize.PLTREL == DT_RELA)
		{
			Elf64_Sxword addend = ((Elf_Rela *)Info->DynamicTable.JMPREL)[i].r_addend;
			Elf_Addr *slot = (Elf_Addr *)(Info->BaseAddress + offset);
			*slot += Info->BaseAddress + addend;
			if (*slot == Info->BaseAddress)
				RelocateHelper(Info, (Elf_Rela *)&((Elf_Rela *)Info->DynamicTable.JMPREL)[i], 0, NULL);
		}
	}

	Info->Flags.Relocated = 1;
	return 0;
}

int _dl_main(int argc, char *argv[], char *envp[])
{
	char *path = argv[0];
	ElfInfo *info = NULL;
	if (sysdep(Access)(path, F_OK) < 0)
	{
		printf("dl: Can't access file %s\n", path);
		return -EACCES;
	}

	int fd = sysdep(Open)(path, O_RDONLY, 0644);
	int status = LoadElf(fd, path, &info);
	if (status < 0)
	{
		printf("%s: Can't load ELF file\n", path);
		sysdep(Close)(fd);
		return status;
	}

	status = HandleRelocations(info);
	if (status < 0)
	{
		printf("%s: Can't relocate ELF file\n", path);
		sysdep(Close)(fd);
		return status;
	}

	sysdep(Close)(fd);
	Elf_Addr entry = info->BaseAddress + info->Header.e_entry;
	return ((int (*)(int, char *[], char *[]))entry)(argc, argv, envp);
}
