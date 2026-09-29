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
	std::vector<Elf_Dyn> ELFGetDynamicTag(Node &fd, DynamicArrayTags Tag)
	{
		std::vector<Elf_Dyn> ret;
		std::vector<Elf_Phdr> phdrs = ELFGetSymbolType(fd, PT_DYNAMIC);

		if (phdrs.empty())
		{
			debug("No dynamic phdrs found.");
			return ret;
		}

		for (auto phdr : phdrs)
		{
			Elf_Dyn dyn;
			for (size_t i = 0; i < phdr.p_filesz / sizeof(Elf_Dyn); i++)
			{
				fs->Read(fd, &dyn, sizeof(Elf_Dyn), phdr.p_offset + (i * sizeof(Elf_Dyn)));
				if (dyn.d_tag != Tag)
					continue;

				debug("Found dynamic tag %d at %#lx [d_val: %#lx]", Tag, &dyn, dyn.d_un.d_val);
				ret.push_back(dyn);
			}
		}

		return ret;
	}
}
