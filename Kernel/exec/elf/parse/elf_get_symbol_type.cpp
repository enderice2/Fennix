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

#include <exec.hpp>

#include "../../../../kernel.h"

namespace Execute
{
	std::vector<Elf_Phdr> ELFGetSymbolType(Node &fd, SegmentTypes Tag)
	{
		std::vector<Elf_Phdr> ret;

		Elf_Ehdr ehdr;
		fs->Read(fd, &ehdr, sizeof(Elf_Ehdr), 0);

		Elf_Phdr phdr;
		fs->Read(fd, &phdr, sizeof(Elf_Phdr), ehdr.e_phoff);

		off_t off = ehdr.e_phoff;
		for (Elf_Half i = 0; i < ehdr.e_phnum; i++)
		{
			if (phdr.p_type == Tag)
				ret.push_back(phdr);

			off += sizeof(Elf_Phdr);
			fs->Read(fd, &phdr, sizeof(Elf_Phdr), off);
		}

		return ret;
	}
}
