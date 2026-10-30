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

#ifndef __FENNIX_KERNEL_non_constructor_tests_H__
#define __FENNIX_KERNEL_non_constructor_tests_H__
#ifdef DEBUG

#include <types.h>
#include <fs/vfs.hpp>

void Test_stl();
void TestMemoryAllocation();
void tasking_test_fb();
void tasking_test_mutex();
void TaskMgr();
void TreeFS(Node node, int Depth);
void TaskHeartbeat();
void StressKernel();
void coroutineTest();
void __early_playground();
void __late_playground();
void test_kernel_fb_speed();

#endif // DEBUG
#endif // !__FENNIX_KERNEL_non_constructor_tests_H__
