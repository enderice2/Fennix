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

// #include <time.hpp>

// #include <memory.hpp>
// #include <acpi.hpp>
// #include <debug.h>
// #include <io.h>

// #include "../../kernel.h"

// #define KVM_CLOCK_PAIRING_WALLCLOCK 0

// namespace Time
// {
// 	class KVMClock
// 	{
// 	private:
// 		struct kvm_clock_pairing
// 		{
// 			int64_t sec;
// 			int64_t nsec;
// 			uint64_t tsc;
// 			uint32_t flags;
// 			uint32_t pad[9];
// 		};

// 		struct pvclock_vcpu_time_info
// 		{
// 			uint32_t version;
// 			uint32_t pad0;
// 			uint64_t tsc_timestamp;
// 			uint64_t system_time;
// 			uint32_t tsc_to_system_mul;
// 			int8_t tsc_shift;
// 			uint8_t flags;
// 			uint8_t pad[2];
// 		};

// 		struct ms_hyperv_tsc_page
// 		{
// 			volatile uint32_t tsc_sequence;
// 			uint32_t reserved1;
// 			volatile uint64_t tsc_scale;
// 			volatile int64_t tsc_offset;
// 			uint64_t reserved2[509];
// 		};

// 		uint64_t clk = 0;
// 		kvm_clock_pairing *Pairing = nullptr;

// 	public:
// 		const char *Name() const override { return "KVM"; }
// 		bool IsAvailable() const override { return clk != 0; }
// 		bool SupportsNanoseconds() const override { return true; }
// 		bool Sleep(uint64_t Nanoseconds) override;
// 		uint64_t GetNanoseconds() override;

// 		KVMClock();
// 		~KVMClock();
// 	};

// 	extern "C" void kvm_hc_clock_pairing(uint64_t phys_addr, uint64_t clock_type)
// 	{
// #if defined(__amd64__)
// 		asm volatile(
// 			"mov $9, %%eax\n\t" /* KVM_HC_CLOCK_PAIRING */
// 			"mov %%rdi, %%rbx\n\t"
// 			"mov %%rsi, %%rcx\n\t"
// 			"vmcall\n\t"
// 			:
// 			: "D"(phys_addr), "S"(clock_type)
// 			: "rax", "rbx", "rcx");
// #else
// #warning "KVM clock pairing not implemented for this architecture"
// #endif
// 	}

// 	bool KVMClock::Sleep(uint64_t Nanoseconds)
// 	{
// 		return true;
// 	}

// 	uint64_t KVMClock::GetNanoseconds()
// 	{
// 		return 0;
// 	}

// 	KVMClock::KVMClock()
// 	{
// 		if (strcmp(CPU::Hypervisor(), x86_CPUID_VENDOR_KVM) != 0)
// 			return;

// 		this->Pairing = (kvm_clock_pairing *)KernelAllocator.RequestPages(TO_PAGES(sizeof(kvm_clock_pairing)));
// 		kvm_hc_clock_pairing((uint64_t)this->Pairing, KVM_CLOCK_PAIRING_WALLCLOCK);
// 		// klog("sec: %lld, nsec: %lld, tsc: %lld", this->Pairing->sec, this->Pairing->nsec, this->Pairing->tsc);
// 		// klog("flags: %x", this->Pairing->flags);
// 	}

// 	KVMClock::~KVMClock()
// 	{
// 	}
// }
