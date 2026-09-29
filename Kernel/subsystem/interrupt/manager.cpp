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

#include <irq.hpp>

void HandleException(CPU::ExceptionFrame *Frame);

namespace Interrupt
{
	void Manager::RegisterHandler(Handle Callback, uint32_t InterruptLine, void *Context, bool Virtual)
	{
		auto &line = RegisteredLines[InterruptLine];
		line.lock.lock();
		line.Handlers.push_back({Callback, Virtual, Context});
		line.lock.unlock();
	}

	void Manager::UnregisterHandler(Handle Callback, uint32_t InterruptLine)
	{
		auto &line = RegisteredLines[InterruptLine];
		line.lock.lock();
		for (auto itr = line.Handlers.begin(); itr != line.Handlers.end();)
		{
			if (itr->Callback == Callback)
				itr = line.Handlers.erase(itr);
			else
				++itr;
		}
		line.lock.unlock();
	}

	void Manager::UnregisterHandler(Handle Callback)
	{
		for (auto &&i : RegisteredLines)
		{
			i.lock.lock();
			for (auto itr = i.Handlers.begin(); itr != i.Handlers.end();)
			{
				if (itr->Callback == Callback)
					itr = i.Handlers.erase(itr);
				else
					++itr;
			}
			i.lock.unlock();
		}
	}

	void Manager::UnregisterHandler(uint32_t InterruptLine)
	{
		auto &line = RegisteredLines[InterruptLine];
		line.lock.lock();
		line.Handlers.clear();
		line.lock.unlock();
	}

	void Manager::UnregisterAllHandlers()
	{
		for (auto &&i : RegisteredLines)
		{
			i.lock.lock();
			i.Handlers.clear();
			i.lock.unlock();
		}
	}

	hot void Manager::Dispatch(CPU::TrapFrame *Frame)
	{
		class AutoSwitchPageTable
		{
		private:
			void *Original;

		public:
			AutoSwitchPageTable()
			{
#if defined(__amd64__) || defined(__i386__)
				asmv("mov %%cr3, %0" : "=r"(Original));
				if (likely(Original == KernelPageTable))
					return;
				asmv("mov %0, %%cr3" : : "r"(KernelPageTable));
#else
#warning "AutoSwitchPageTable not implemented for this architecture"
#endif
			}

			~AutoSwitchPageTable()
			{
#if defined(__amd64__) || defined(__i386__)
				if (likely(Original == KernelPageTable))
					return;
				asmv("mov %0, %%cr3" : : "r"(Original));
#else
#warning "~AutoSwitchPageTable not implemented for this architecture"
#endif
			}
		} spt;

		if (unlikely(ctrl->TranslateToLine(Frame->InterruptNumber) == IRQ31))
			CPU::Stop();

		/* Corrupted Frame */
		assert(ctrl->TranslateToLine(Frame->InterruptNumber) <= numof(RegisteredLines));

		bool dispatched = false;
		auto &line = RegisteredLines[ctrl->TranslateToLine(Frame->InterruptNumber)];

		/* FIXME: remove spinlock and use RCU or similar */
		line.lock.lock();
		for (auto &handle : line.Handlers)
		{
			if (handle.Callback == nullptr)
				continue;

			CPU::TrapFrame *ctx = handle.Context ? (CPU::TrapFrame *)handle.Context : Frame;
			int ret;
			if (handle.IsVirtualFunction)
				ret = ((Handler *)handle.Callback)->OnInterruptReceived(ctx);
			else
				ret = handle.Callback(ctx);

			if (ret == ENOTSUP)
				continue;
			// if (ret != EOK)
			// {
			// 	if (ret == ENOTSUP)
			// 		continue;

			// 	/* ? */
			// }

			dispatched = true;
		}
		line.lock.unlock();

		if (!dispatched)
		{
			debug("unhandled IRQ %u", ctrl->TranslateToLine(Frame->InterruptNumber));
		}
		ctrl->EOI(Frame->InterruptNumber);
	}

	hot void Manager::Dispatch(CPU::SchedulerFrame *Frame)
	{
		KernelPageTable->Update();
		assert(ctrl->TranslateToLine(Frame->InterruptNumber) == IRQ16);

		auto &line = RegisteredLines[ctrl->TranslateToLine(Frame->InterruptNumber)];

		line.lock.lock();
		for (auto &handle : line.Handlers)
		{
			if (handle.Callback == nullptr)
				continue;

			assert(handle.IsVirtualFunction == true);
			((Handler *)handle.Callback)->OnInterruptReceived(Frame);
		}
		line.lock.unlock();

		ctrl->EOI(Frame->InterruptNumber);
	}

	void Manager::Dispatch(CPU::ExceptionFrame *Frame) { HandleException(Frame); }

	void Manager::Initialize(Controller &controller) { ctrl = &controller; }
}
