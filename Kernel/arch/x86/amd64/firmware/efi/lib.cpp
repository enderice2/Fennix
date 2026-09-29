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

#include <efi.h>

#include <convert.h>

BOOLEAN EFIAPI CompareGuid(IN CONST GUID *Guid1, IN CONST GUID *Guid2)
{
	if (Guid1 == NULL || Guid2 == NULL)
		return FALSE;

	return (Guid1->Data1 == Guid2->Data1 &&
			Guid1->Data2 == Guid2->Data2 &&
			Guid1->Data3 == Guid2->Data3 &&
			memcmp(Guid1->Data4, Guid2->Data4, sizeof(Guid1->Data4)) == 0);
}

VOID InitializeLib(IN EFI_HANDLE, IN EFI_SYSTEM_TABLE *)
{
	/* Does nothing */
}
