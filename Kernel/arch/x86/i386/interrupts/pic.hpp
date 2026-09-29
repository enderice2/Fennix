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

#ifndef __FENNIX_KERNEL_8259PIC_H__
#define __FENNIX_KERNEL_8259PIC_H__

#include <types.h>

namespace PIC
{
	class PIC
	{
	private:
		uint8_t MasterCommandPort;
		uint8_t MasterDataPort;
		uint8_t SlaveCommandPort;
		uint8_t SlaveDataPort;
		uint8_t MasterOffset;
		uint8_t SlaveOffset;
		uint8_t MasterMask;
		uint8_t SlaveMask;

	public:
		PIC(uint8_t MasterCommandPort, uint8_t MasterDataPort, uint8_t SlaveCommandPort, uint8_t SlaveDataPort, uint8_t MasterOffset, uint8_t SlaveOffset);
		~PIC();
		void Mask(uint8_t IRQ);
		void Unmask(uint8_t IRQ);
		void SendEOI(uint8_t IRQ);
	};

	class PIT
	{
	private:
		uint16_t Port;
		uint16_t Frequency;

	public:
		PIT(uint16_t Port, uint16_t Frequency);
		~PIT();
		void PrepareSleep(uint32_t Milliseconds);
		void PerformSleep();
	};
}

#endif // !__FENNIX_KERNEL_8259PIC_H__
