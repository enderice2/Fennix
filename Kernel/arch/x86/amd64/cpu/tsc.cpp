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

#include "tsc.hpp"

#include <memory.hpp>
#include <acpi.hpp>
#include <debug.h>
#include <io.h>

#include "../../../../kernel.h"

using namespace std::chrono_literals;

namespace TimeStampCounter
{
	static inline uint64_t rdtsc()
	{
		unsigned int lo, hi;
		__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
		return ((uint64_t)hi << 32) | lo;
	}

	uint64_t TSC::Now() const
	{
		uint64_t tsc = rdtsc();
		return (tsc * 1'000'000'000ULL) / clk;
	}

	void TSC::Sleep(std::chrono::nanoseconds ns)
	{
		uint64_t target = Now() + ns.count();
		while (Now() < target)
			CPU::Pause();
	}

	void TSC::Calibrate()
	{
		// const int attempts = 5;
		// uint64_t ns = 10'000'000ULL; /* 10 ms */
		// uint64_t total_clk = 0;
		// uint64_t overhead = 0;

		// for (int i = 0; i < attempts; ++i)
		// {
		// 	uint64_t t0 = rdtsc();
		// 	uint64_t t1 = rdtsc();
		// 	overhead += (t1 - t0);
		// }
		// overhead /= attempts;

		// for (int i = 0; i < attempts; ++i)
		// {
		// 	uint64_t tsc_start = rdtsc();
		// 	uint64_t clock_start = thisClock->Now();
		// 	while (thisClock->Now() - clock_start < ns)
		// 		CPU::Pause();
		// 	uint64_t tsc_end = rdtsc();
		// 	total_clk += (tsc_end - tsc_start - overhead) * 1'000'000'000ULL / ns;
		// }
		// this->clk = total_clk / attempts;
		// klog("TSC frequency: %lu MHz", this->clk / 1'000'000);

		constexpr uint32_t test_ms = 10;
		uint64_t start = rdtsc();
		GlobalClock->Sleep(10ms);
		uint64_t end = rdtsc();

		uint64_t delta = end - start;

		// scale to Hz
		this->clk = (delta * 1000) / test_ms;
	}

	bool TSC::Invariant()
	{
		if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_AMD) == 0)
		{
			CPU::x86::AMD::CPUID0x80000007 cpuid80000007;
			return cpuid80000007.EDX.TscInvariant;
		}
		else if (strcmp(CPU::Vendor(), x86_CPUID_VENDOR_INTEL) == 0)
		{
			// TODO: Intel 0x80000007
			CPU::x86::AMD::CPUID0x80000007 cpuid80000007;
			return cpuid80000007.EDX.TscInvariant;
		}

		return false;
	}
}
