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

#pragma once

#include <types.h>

#ifdef __cplusplus

namespace Log
{
#define LOG_MESSAGE_MAX 256

	enum LogLevel
	{
		LogLevelNone = 0,
		LogLevelError,
		LogLevelWarning,
		LogLevelFixme,
		LogLevelInfo,
		LogLevelStub,
		LogLevelDebug,
		LogLevelUbsan,
		LogLevelFunction,
	};

	struct LogRecord
	{
		uint64_t Sequence;
		uint64_t TimestampNs;

		uint8_t CpuID;
		pid_t ThreadID;

		LogLevel Level;

		const char *File;
		const char *Function;
		uint32_t Line;

		uint16_t MessageLength;
		char Message[LOG_MESSAGE_MAX];
	};

	void printk(LogLevel Level, const char *File, int Line, const char *Function, const char *Format, ...);

	int RegisterSink(void (*Write)(const LogRecord *));
	int UnregisterSink(void (*Write)(const LogRecord *));

	/* very basic */
	void EarlyInit();

	/* allocates from heap memory in CPUData, called per-core */
	void MemoryInit(int CoreID = 0);

	/* after everything is done, (sched, vfs, etc...) */
	void LateInit();

	void uart_wrapper(char c, void *);
}

#define error(fmt, ...) Log::printk(Log::LogLevel::LogLevelError, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...) Log::printk(Log::LogLevel::LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define fixme(fmt, ...) Log::printk(Log::LogLevel::LogLevelFixme, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define info(fmt, ...) Log::printk(Log::LogLevel::LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define klog(fmt, ...) Log::printk(Log::LogLevel::LogLevelNone, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define stub Log::printk(Log::LogLevel::LogLevelStub, __FILE__, __LINE__, __FUNCTION__, " ")

#define trace(fmt, ...) klog(fmt, ##__VA_ARGS__)

#ifdef DEBUG
#define debug(fmt, ...) Log::printk(Log::LogLevel::LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ubsan(fmt, ...) Log::printk(Log::LogLevel::LogLevelUbsan, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define func(fmt, ...) Log::printk(Log::LogLevel::LogLevelFunction, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#define ubsan(fmt, ...)
#define func(fmt, ...)
#endif

#else // __cplusplus

void __c_kprint_error(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_warning(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_info(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_debug(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_ubsan(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_func(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_fixme(const char *File, int Line, const char *Function, const char *Format, ...);
void __c_kprint_stub(const char *File, int Line, const char *Function);

#define error(fmt, ...) __c_kprint_error(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...) __c_kprint_warning(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define info(fmt, ...) __c_kprint_info(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define fixme(fmt, ...) __c_kprint_fixme(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define stub __c_kprint_stub(__FILE__, __LINE__, __FUNCTION__)

#ifdef DEBUG
#define debug(fmt, ...) __c_kprint_debug(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ubsan(fmt, ...) __c_kprint_ubsan(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define func(fmt, ...) __c_kprint_func(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#define ubsan(fmt, ...)
#define func(fmt, ...)
#endif

#endif // __cplusplus
