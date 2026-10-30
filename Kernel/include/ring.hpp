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

#ifndef __FENNIX_KERNEL_RING_BUFFER_H__
#define __FENNIX_KERNEL_RING_BUFFER_H__

#include <types.h>
#include <lock.hpp>

/**
 * @brief A generic ring buffer implementation with thread safety.
 *
 * @tparam T Type of the data stored in the buffer.
 */
template <typename T>
class RingBuffer
{
private:
	LockClass Lock;
	T *Buffer;
	size_t BufferSize;
	size_t BufferCount;

	size_t Head;
	size_t Tail;

public:
	/**
	 * @brief Construct a new RingBuffer object.
	 *
	 * Initializes the buffer and the spin lock. The default buffer size is 16.
	 *
	 * @param Size Initial size (capacity) of the buffer.
	 */
	RingBuffer(size_t Size = 16)
		: Lock(),
		  Buffer(new T[Size]),
		  BufferSize(Size),
		  BufferCount(0),
		  Head(0),
		  Tail(0) {}

	/**
	 * @brief Destroy the RingBuffer object.
	 *
	 * Releases the allocated memory for the buffer and the spin lock.
	 */
	~RingBuffer()
	{
		SmartCriticalSection(Lock);
		delete[] Buffer;
	}

	/**
	 * @brief Write data to the ring buffer.
	 *
	 * Writes up to `nReads` elements from `Data` into the buffer. If the buffer
	 * is full, the function stops writing.
	 *
	 * @param Data Pointer to the data to be written.
	 * @param nReads Number of elements to write.
	 * @return size_t Number of elements actually written.
	 */
	size_t Write(const T *Data, size_t nReads)
	{
		SmartCriticalSection(Lock);

		size_t written = 0;
		while (nReads > 0)
		{
			if (BufferCount == BufferSize)
				break;

			Buffer[Head] = *Data++;
			Head = (Head + 1) % BufferSize;
			BufferCount++;
			written++;
			nReads--;
		}

		return written;
	}

	/**
	 * @brief Read data from the ring buffer.
	 *
	 * Reads up to `nReads` elements from the buffer into `Data`. If the buffer
	 * is empty, the function stops reading.
	 *
	 * @param Data Pointer to the memory where read elements will be stored.
	 * @param nReads Number of elements to read.
	 * @return size_t Number of elements actually read.
	 */
	size_t Read(T *Data, size_t nReads)
	{
		SmartCriticalSection(Lock);

		size_t read = 0;
		while (nReads > 0)
		{
			if (BufferCount == 0)
				break;

			*Data++ = Buffer[Tail];
			Tail = (Tail + 1) % BufferSize;
			BufferCount--;
			read++;
			nReads--;
		}

		return read;
	}

	/**
	 * @brief Peek at data in the ring buffer without removing it.
	 *
	 * Peeks up to `nReads` elements from the buffer into `Data` without
	 * modifying the buffer state. If the buffer is empty, the function stops peeking.
	 *
	 * @param Data Pointer to the memory where peeked elements will be stored.
	 * @param nReads Number of elements to peek.
	 * @return size_t Number of elements actually peeked.
	 */
	size_t Peek(T *Data, size_t nReads)
	{
		SmartCriticalSection(Lock);

		size_t read = 0;
		size_t tail = Tail;
		while (nReads > 0)
		{
			if (BufferCount == 0)
				break;

			*Data++ = Buffer[tail];
			tail = (tail + 1) % BufferSize;
			read++;
			nReads--;
		}

		return read;
	}

	/**
	 * @brief Get the current number of elements in the buffer.
	 *
	 * @return size_t The number of elements in the buffer.
	 */
	size_t Count()
	{
		SmartCriticalSection(Lock);
		return BufferCount;
	}

	/**
	 * @brief Get the available space left in the buffer.
	 *
	 * @return size_t The number of free slots in the buffer.
	 */
	size_t Free()
	{
		SmartCriticalSection(Lock);
		return BufferSize - BufferCount;
	}
};

#endif // !__FENNIX_KERNEL_RING_BUFFER_H__
