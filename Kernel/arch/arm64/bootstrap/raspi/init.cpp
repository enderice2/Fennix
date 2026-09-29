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
#include <cpu.hpp>

#include "../../../../kernel.h"

using namespace CPU::aarch64;

extern "C" void KernelEntry(struct BootInfo *Info);

extern "C" __attribute__((section(".bootstrap.text"))) void _aarch64_start(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
{
	MIDR_EL1 reg;
	asmv("mrs %x0, midr_el1" : "=r"(reg.raw));

	switch (reg.PartNum)
	{
	case 0xB76: /* Raspberry Pi 1 */
	case 0xC07: /* Raspberry Pi 2 */
	default:	/* Unknown */
		CPU::Stop();
	case 0xD03: /* Raspberry Pi 3 */
		break;
	case 0xD08: /* Raspberry Pi 4 */
		break;
	}

	BootInfo *info = nullptr;
	KernelEntry(info);
}
