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

#ifdef DEBUG

#include "t.h"

#include "../kernel.h"

void TaskHeartbeat()
{
	thisThread->Rename("Task Heartbeat");
	thisThread->SetPriority(Tasking::Idle);

	while (true)
	{
		debug("Task Heartbeat %llu", GlobalClock->Now());
		TaskManager->Sleep(5000 * 1'000'000ULL);
	}
}

#endif // DEBUG
