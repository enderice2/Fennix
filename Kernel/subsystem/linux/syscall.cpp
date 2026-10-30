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

#include <syscalls.hpp>

#include <static_vector>
#include <signal.hpp>
#include <dumper.hpp>
#include <utsname.h>
#include <rand.hpp>
#include <limits.h>
#include <exec.hpp>
#include <task.hpp>
#include <debug.h>
#include <cpu.hpp>
#include <time.h>

#include <memory.hpp>
#define INI_IMPLEMENTATION
#include <ini.h>

#include "include/syscalls_amd64.hpp"
#include "include/syscalls_i386.hpp"
#include "include/signals.hpp"
#include "include/defs.hpp"
#include "include/errno.h"

#include "../../kernel.h"

using Tasking::PCB;
using Tasking::TCB;

static_assert(linux_SIGRTMIN == SIGRTMIN);
static_assert(linux_SIGRTMAX == SIGRTMAX);

struct SyscallData
{
	const char *Name;
	void *Handler;
};

#ifdef DEBUG
const char *lSigStr[] = {
	"INVALID",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGBUS",
	"SIGFPE",
	"SIGKILL",
	"SIGUSR1",
	"SIGSEGV",
	"SIGUSR2",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGSTKFLT",
	"SIGCHLD",
	"SIGCONT",
	"SIGSTOP",
	"SIGTSTP",
	"SIGTTIN",
	"SIGTTOU",
	"SIGURG",
	"SIGXCPU",
	"SIGXFSZ",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGPOLL",
	"SIGPWR",
	"SIGSYS",
	"SIGRTMIN",
	"SIGRT_1",
	"SIGRT_2",
	"SIGRT_3",
	"SIGRT_4",
	"SIGRT_5",
	"SIGRT_6",
	"SIGRT_7",
	"SIGRT_8",
	"SIGRT_9",
	"SIGRT_10",
	"SIGRT_11",
	"SIGRT_12",
	"SIGRT_13",
	"SIGRT_14",
	"SIGRT_15",
	"SIGRT_16",
	"SIGRT_17",
	"SIGRT_18",
	"SIGRT_19",
	"SIGRT_20",
	"SIGRT_21",
	"SIGRT_22",
	"SIGRT_23",
	"SIGRT_24",
	"SIGRT_25",
	"SIGRT_26",
	"SIGRT_27",
	"SIGRT_28",
	"SIGRT_29",
	"SIGRT_30",
	"SIGRT_31",
	"SIGRTMAX",
};

const char *nErrnoStr[] = {
	"EOK",
	"E2BIG",
	"EACCES",
	"EADDRINUSE",
	"EADDRNOTAVAIL",
	"EAFNOSUPPORT",
	"EAGAIN",
	"EALREADY",
	"EBADF",
	"EBADMSG",
	"EBUSY",
	"ECANCELED",
	"ECHILD",
	"ECONNABORTED",
	"ECONNREFUSED",
	"ECONNRESET",
	"EDEADLK",
	"EDESTADDRREQ",
	"EDOM",
	"EDQUOT",
	"EEXIST",
	"EFAULT",
	"EFBIG",
	"EHOSTUNREACH",
	"EIDRM",
	"EILSEQ",
	"EINPROGRESS",
	"EINTR",
	"EINVAL",
	"EIO",
	"EISCONN",
	"EISDIR",
	"ELOOP",
	"EMFILE",
	"EMLINK",
	"EMSGSIZE",
	"EMULTIHOP",
	"ENAMETOOLONG",
	"ENETDOWN",
	"ENETRESET",
	"ENETUNREACH",
	"ENFILE",
	"ENOBUFS",
	"ENODATA",
	"ENODEV",
	"ENOENT",
	"ENOEXEC",
	"ENOLCK",
	"ENOLINK",
	"ENOMEM",
	"ENOMSG",
	"ENOPROTOOPT",
	"ENOSPC",
	"ENOSR",
	"ENOSTR",
	"ENOSYS",
	"ENOTCONN",
	"ENOTDIR",
	"ENOTEMPTY",
	"ENOTRECOVERABLE",
	"ENOTSOCK",
	"ENOTSUP",
	"ENOTTY",
	"ENXIO",
	"EOPNOTSUPP",
	"EOVERFLOW",
	"EOWNERDEAD",
	"EPERM",
	"EPIPE",
	"EPROTO",
	"EPROTONOSUPPORT",
	"EPROTOTYPE",
	"ERANGE",
	"EROFS",
	"ESPIPE",
	"ESRCH",
	"ESTALE",
	"ETIME",
	"ETIMEDOUT",
	"ETXTBSY",
	"EWOULDBLOCK",
	"EXDEV",
};

const char *lErrnoStr[] = {
	"zero",
	"EPERM",
	"ENOENT",
	"ESRCH",
	"EINTR",
	"EIO",
	"ENXIO",
	"E2BIG",
	"ENOEXEC",
	"EBADF",
	"ECHILD",
	"EAGAIN",
	"ENOMEM",
	"EACCES",
	"EFAULT",
	"ENOTBLK",
	"EBUSY",
	"EEXIST",
	"EXDEV",
	"ENODEV",
	"ENOTDIR",
	"EISDIR",
	"EINVAL",
	"ENFILE",
	"EMFILE",
	"ENOTTY",
	"ETXTBSY",
	"EFBIG",
	"ENOSPC",
	"ESPIPE",
	"EROFS",
	"EMLINK",
	"EPIPE",
	"EDOM",
	"ERANGE",
	"EDEADLK",
	"ENAMETOOLONG",
	"ENOLCK",
	"ENOSYS",
	"ENOTEMPTY",
	"ELOOP",
	"unknown",
	"ENOMSG",
	"EIDRM",
	"ECHRNG",
	"EL2NSYNC",
	"EL3HLT",
	"EL3RST",
	"ELNRNG",
	"EUNATCH",
	"ENOCSI",
	"EL2HLT",
	"EBADE",
	"EBADR",
	"EXFULL",
	"ENOANO",
	"EBADRQC",
	"EBADSLT",
	"unknown",
	"EBFONT",
	"ENOSTR",
	"ENODATA",
	"ETIME",
	"ENOSR",
	"ENONET",
	"ENOPKG",
	"EREMOTE",
	"ENOLINK",
	"EADV",
	"ESRMNT",
	"ECOMM",
	"EPROTO",
	"EMULTIHOP",
	"EDOTDOT",
	"EBADMSG",
	"EOVERFLOW",
	"ENOTUNIQ",
	"EBADFD",
	"EREMCHG",
	"ELIBACC",
	"ELIBBAD",
	"ELIBSCN",
	"ELIBMAX",
	"ELIBEXEC",
	"EILSEQ",
	"ERESTART",
	"ESTRPIPE",
	"EUSERS",
	"ENOTSOCK",
	"EDESTADDRREQ",
	"EMSGSIZE",
	"EPROTOTYPE",
	"ENOPROTOOPT",
	"EPROTONOSUPPORT",
	"ESOCKTNOSUPPORT",
	"EOPNOTSUPP",
	"EPFNOSUPPORT",
	"EAFNOSUPPORT",
	"EADDRINUSE",
	"EADDRNOTAVAIL",
	"ENETDOWN",
	"ENETUNREACH",
	"ENETRESET",
	"ECONNABORTED",
	"ECONNRESET",
	"ENOBUFS",
	"EISCONN",
	"ENOTCONN",
	"ESHUTDOWN",
	"ETOOMANYREFS",
	"ETIMEDOUT",
	"ECONNREFUSED",
	"EHOSTDOWN",
	"EHOSTUNREACH",
	"EALREADY",
	"EINPROGRESS",
	"ESTALE",
	"EUCLEAN",
	"ENOTNAM",
	"ENAVAIL",
	"EISNAM",
	"EREMOTEIO",
	"EDQUOT",
	"ENOMEDIUM",
	"EMEDIUMTYPE",
	"ECANCELED",
	"ENOKEY",
	"EKEYEXPIRED",
	"EKEYREVOKED",
	"EKEYREJECTED",
	"EOWNERDEAD",
	"ENOTRECOVERABLE",
};
#endif

const char *rlimitStr[] = {
	"RLIMIT_CPU",
	"RLIMIT_FSIZE",
	"RLIMIT_DATA",
	"RLIMIT_STACK",
	"RLIMIT_CORE",
	"RLIMIT_RSS",
	"RLIMIT_NPROC",
	"RLIMIT_NOFILE",
	"RLIMIT_MEMLOCK",
	"RLIMIT_AS",
	"RLIMIT_LOCKS",
	"RLIMIT_SIGPENDING",
	"RLIMIT_MSGQUEUE",
	"RLIMIT_NICE",
	"RLIMIT_RTPRIO",
	"RLIMIT_RTTIME",
	"RLIMIT_NLIMITS",
};

static const struct
{
	int linuxSignal;
	signal_t nativeSignal;
	signal_disposition_t nativeDisposition;
} signalMapping[] = {
	{linux_SIGHUP, SIGHUP, SIG_TERM},
	{linux_SIGINT, SIGINT, SIG_TERM},
	{linux_SIGQUIT, SIGQUIT, SIG_TERM},
	{linux_SIGILL, SIGILL, SIG_CORE},
	{linux_SIGTRAP, SIGTRAP, SIG_CORE},
	{linux_SIGABRT, SIGABRT, SIG_CORE},
	{linux_SIGBUS, SIGBUS, SIG_CORE},
	{linux_SIGFPE, SIGFPE, SIG_CORE},
	{linux_SIGKILL, SIGKILL, SIG_TERM},
	{linux_SIGUSR1, SIGUSR1, SIG_TERM},
	{linux_SIGSEGV, SIGSEGV, SIG_CORE},
	{linux_SIGUSR2, SIGUSR2, SIG_TERM},
	{linux_SIGPIPE, SIGPIPE, SIG_TERM},
	{linux_SIGALRM, SIGALRM, SIG_TERM},
	{linux_SIGTERM, SIGTERM, SIG_TERM},
	{linux_SIGSTKFLT, SIGCOMP1, SIG_IGN},
	{linux_SIGCHLD, SIGCHLD, SIG_IGN},
	{linux_SIGCONT, SIGCONT, SIG_CONT},
	{linux_SIGSTOP, SIGSTOP, SIG_STOP},
	{linux_SIGTSTP, SIGTSTP, SIG_STOP},
	{linux_SIGTTIN, SIGTTIN, SIG_STOP},
	{linux_SIGTTOU, SIGTTOU, SIG_STOP},
	{linux_SIGURG, SIGURG, SIG_IGN},
	{linux_SIGXCPU, SIGXCPU, SIG_CORE},
	{linux_SIGXFSZ, SIGXFSZ, SIG_CORE},
	{linux_SIGVTALRM, SIGVTALRM, SIG_TERM},
	{linux_SIGPROF, SIGPROF, SIG_TERM},
	{linux_SIGWINCH, SIGCOMP2, SIG_IGN},
	{linux_SIGPOLL, SIGPOLL, SIG_TERM},
	{linux_SIGPWR, SIGCOMP3, SIG_IGN},
	{linux_SIGSYS, SIGSYS, SIG_CORE},
	{linux_SIGRTMIN, SIGRTMIN, SIG_IGN},
	{linux_SIGRTMIN + 1, SIGRT_1, SIG_IGN},
	{linux_SIGRTMIN + 2, SIGRT_2, SIG_IGN},
	{linux_SIGRTMIN + 3, SIGRT_3, SIG_IGN},
	{linux_SIGRTMIN + 4, SIGRT_4, SIG_IGN},
	{linux_SIGRTMIN + 5, SIGRT_5, SIG_IGN},
	{linux_SIGRTMIN + 6, SIGRT_6, SIG_IGN},
	{linux_SIGRTMIN + 7, SIGRT_7, SIG_IGN},
	{linux_SIGRTMIN + 8, SIGRT_8, SIG_IGN},
	{linux_SIGRTMIN + 9, SIGRT_9, SIG_IGN},
	{linux_SIGRTMIN + 10, SIGRT_10, SIG_IGN},
	{linux_SIGRTMIN + 11, SIGRT_11, SIG_IGN},
	{linux_SIGRTMIN + 12, SIGRT_12, SIG_IGN},
	{linux_SIGRTMIN + 13, SIGRT_13, SIG_IGN},
	{linux_SIGRTMIN + 14, SIGRT_14, SIG_IGN},
	{linux_SIGRTMIN + 15, SIGRT_15, SIG_IGN},
	{linux_SIGRTMIN + 16, SIGRT_16, SIG_IGN},
	{linux_SIGRTMIN + 17, SIGRT_17, SIG_IGN},
	{linux_SIGRTMIN + 18, SIGRT_18, SIG_IGN},
	{linux_SIGRTMIN + 19, SIGRT_19, SIG_IGN},
	{linux_SIGRTMIN + 20, SIGRT_20, SIG_IGN},
	{linux_SIGRTMIN + 21, SIGRT_21, SIG_IGN},
	{linux_SIGRTMIN + 22, SIGRT_22, SIG_IGN},
	{linux_SIGRTMIN + 23, SIGRT_23, SIG_IGN},
	{linux_SIGRTMIN + 24, SIGRT_24, SIG_IGN},
	{linux_SIGRTMIN + 25, SIGRT_25, SIG_IGN},
	{linux_SIGRTMIN + 26, SIGRT_26, SIG_IGN},
	{linux_SIGRTMIN + 27, SIGRT_27, SIG_IGN},
	{linux_SIGRTMIN + 28, SIGRT_28, SIG_IGN},
	{linux_SIGRTMIN + 29, SIGRT_29, SIG_IGN},
	{linux_SIGRTMIN + 30, SIGRT_30, SIG_IGN},
	{linux_SIGRTMIN + 31, SIGRT_31, SIG_IGN},
	{linux_SIGRTMAX, SIGRTMAX, SIG_IGN},
};

static_vector<int, __ERRNO_MAX> errnoMap = {
	/*EOK*/ 0,
	/*E2BIG*/ linux_E2BIG,
	/*EACCES*/ linux_EACCES,
	/*EADDRINUSE*/ linux_EADDRINUSE,
	/*EADDRNOTAVAIL*/ linux_EADDRNOTAVAIL,
	/*EAFNOSUPPORT*/ linux_EAFNOSUPPORT,
	/*EAGAIN*/ linux_EAGAIN,
	/*EALREADY*/ linux_EALREADY,
	/*EBADF*/ linux_EBADF,
	/*EBADMSG*/ linux_EBADMSG,
	/*EBUSY*/ linux_EBUSY,
	/*ECANCELED*/ linux_ECANCELED,
	/*ECHILD*/ linux_ECHILD,
	/*ECONNABORTED*/ linux_ECONNABORTED,
	/*ECONNREFUSED*/ linux_ECONNREFUSED,
	/*ECONNRESET*/ linux_ECONNRESET,
	/*EDEADLK*/ linux_EDEADLK,
	/*EDESTADDRREQ*/ linux_EDESTADDRREQ,
	/*EDOM*/ linux_EDOM,
	/*EDQUOT*/ linux_EDQUOT,
	/*EEXIST*/ linux_EEXIST,
	/*EFAULT*/ linux_EFAULT,
	/*EFBIG*/ linux_EFBIG,
	/*EHOSTUNREACH*/ linux_EHOSTUNREACH,
	/*EIDRM*/ linux_EIDRM,
	/*EILSEQ*/ linux_EILSEQ,
	/*EINPROGRESS*/ linux_EINPROGRESS,
	/*EINTR*/ linux_EINTR,
	/*EINVAL*/ linux_EINVAL,
	/*EIO*/ linux_EIO,
	/*EISCONN*/ linux_EISCONN,
	/*EISDIR*/ linux_EISDIR,
	/*ELOOP*/ linux_ELOOP,
	/*EMFILE*/ linux_EMFILE,
	/*EMLINK*/ linux_EMLINK,
	/*EMSGSIZE*/ linux_EMSGSIZE,
	/*EMULTIHOP*/ linux_EMULTIHOP,
	/*ENAMETOOLONG*/ linux_ENAMETOOLONG,
	/*ENETDOWN*/ linux_ENETDOWN,
	/*ENETRESET*/ linux_ENETRESET,
	/*ENETUNREACH*/ linux_ENETUNREACH,
	/*ENFILE*/ linux_ENFILE,
	/*ENOBUFS*/ linux_ENOBUFS,
	/*ENODATA*/ linux_ENODATA,
	/*ENODEV*/ linux_ENODEV,
	/*ENOENT*/ linux_ENOENT,
	/*ENOEXEC*/ linux_ENOEXEC,
	/*ENOLCK*/ linux_ENOLCK,
	/*ENOLINK*/ linux_ENOLINK,
	/*ENOMEM*/ linux_ENOMEM,
	/*ENOMSG*/ linux_ENOMSG,
	/*ENOPROTOOPT*/ linux_ENOPROTOOPT,
	/*ENOSPC*/ linux_ENOSPC,
	/*ENOSR*/ linux_ENOSR,
	/*ENOSTR*/ linux_ENOSTR,
	/*ENOSYS*/ linux_ENOSYS,
	/*ENOTCONN*/ linux_ENOTCONN,
	/*ENOTDIR*/ linux_ENOTDIR,
	/*ENOTEMPTY*/ linux_ENOTEMPTY,
	/*ENOTRECOVERABLE*/ linux_ENOTRECOVERABLE,
	/*ENOTSOCK*/ linux_ENOTSOCK,
	/*ENOTSUP*/ linux_EOPNOTSUPP,
	/*ENOTTY*/ linux_ENOTTY,
	/*ENXIO*/ linux_ENXIO,
	/*EOPNOTSUPP*/ linux_EOPNOTSUPP,
	/*EOVERFLOW*/ linux_EOVERFLOW,
	/*EOWNERDEAD*/ linux_EOWNERDEAD,
	/*EPERM*/ linux_EPERM,
	/*EPIPE*/ linux_EPIPE,
	/*EPROTO*/ linux_EPROTO,
	/*EPROTONOSUPPORT*/ linux_EPROTONOSUPPORT,
	/*EPROTOTYPE*/ linux_EPROTOTYPE,
	/*ERANGE*/ linux_ERANGE,
	/*EROFS*/ linux_EROFS,
	/*ESPIPE*/ linux_ESPIPE,
	/*ESRCH*/ linux_ESRCH,
	/*ESTALE*/ linux_ESTALE,
	/*ETIME*/ linux_ETIME,
	/*ETIMEDOUT*/ linux_ETIMEDOUT,
	/*ETXTBSY*/ linux_ETXTBSY,
	/*EWOULDBLOCK*/ linux_EAGAIN,
	/*EXDEV*/ linux_EXDEV,
};

void KST_Reboot() { PowerManager->Reboot(); }
void KST_Shutdown() { PowerManager->Shutdown(); }

inline uint64_t FromSeconds(uint64_t Seconds) { return Seconds * 1'000'000'000ULL; }
inline uint64_t FromMilliseconds(uint64_t Milliseconds) { return Milliseconds * 1'000'000ULL; }

inline uint64_t ToSeconds(uint64_t Nanoseconds) { return Nanoseconds / 1'000'000'000ULL; }
inline uint64_t ToMilliseconds(uint64_t Nanoseconds) { return Nanoseconds / 1'000'000ULL; }

inline intptr_t ConvertErrnoToLinux(auto err)
{
	intptr_t errc = (intptr_t)err;

	if (errc >= 0)
	{
		debug("no change for %#lx", errc);
		return errc;
	}

	intptr_t ret = errnoMap[-errc];
	debug("converted %s(%ld) to %s(%ld)", nErrnoStr[-errc], -errc, lErrnoStr[ret], ret);
	return -ret;
}

int ConvertSignalToLinux(signal_t sig)
{
	if (sig >= SIGRTMIN && sig <= SIGRTMAX)
		return sig; /* We ignore for now */

	for (auto &mapping : signalMapping)
	{
		if (mapping.nativeSignal == sig)
		{
			// debug("Converted \"%s\"(%d) to \"%s\"(%d)",
			// 	  sigStr[mapping.nativeSignal], sig,
			// 	  lSigStr[mapping.linuxSignal], mapping.linuxSignal);
			return mapping.linuxSignal;
		}
	}
	debug("Unknown signal %d", sig);
	// assert(!"Unknown signal");
	return SIGNULL;
}

signal_t ConvertSignalToNative(int sig)
{
	if (sig >= linux_SIGRTMIN && sig <= linux_SIGRTMAX)
		return (signal_t)sig; /* We ignore for now */

	for (auto &mapping : signalMapping)
	{
		if (mapping.linuxSignal == sig)
		{
			// debug("Converted \"%s\"(%d) to \"%s\"(%d)",
			// 	  lSigStr[mapping.linuxSignal], sig,
			// 	  sigStr[mapping.nativeSignal], mapping.nativeSignal);
			return mapping.nativeSignal;
		}
	}
	debug("Unknown signal %d", sig);
	// assert(!"Unknown signal");
	return SIGNULL;
}

unsigned long ConvertMaskToNative(unsigned long mask)
{
	unsigned long ret = 0;
	for (int i = 0; i < 64; i++)
	{
		if (mask & (1ul << i))
		{
			int sig = ConvertSignalToNative(i + 1);
			if (unlikely(sig == SIGNULL))
				continue;
			ret |= 1ul << (sig - 1);
		}
	}
	return ret;

	// std::bitset<SIGNAL_MAX + 1> bitMask(mask);
	// for (int i = 0; i < SIGNAL_MAX + 1; i++)
	// {
	// 	if (bitMask.test(i))
	// 		ret |= 1 << (ConvertSignalToNative(i + 1) - 1);
	// }
}

unsigned long ConvertMaskToLinux(unsigned long mask)
{
	unsigned long ret = 0;
	for (int i = 0; i < 64; i++)
	{
		if (mask & (1ul << i))
		{
			int sig = ConvertSignalToLinux((signal_t)(i + 1));
			if (unlikely(sig == SIGNULL))
				continue;
			ret |= 1ul << (sig - 1);
		}
	}
	return ret;

	// std::bitset<linux_SIGUNUSED + 1> bitMask(mask);
	// for (int i = 0; i < linux_SIGUNUSED + 1; i++)
	// {
	// 	if (bitMask.test(i))
	// 		ret |= 1 << (ConvertSignalToLinux(i + 1) - 1);
	// }
}

void SetSigActToNative(const k_sigaction *linux, SignalAction *native)
{
	native->sa_handler.Handler = linux->handler;
	native->Flags = linux->flags;
	native->Restorer = linux->restorer;

#if defined(__amd64__)
	unsigned long mask = ((unsigned long)linux->mask[1] << 32) | linux->mask[0];
	native->Mask = std::bitset<64>(ConvertMaskToNative(mask));
	debug("m0:%#lx m1:%#lx | n:%#lx", linux->mask[0], linux->mask[1], native->Mask);
#elif defined(__i386__)
#warning "SetSigActToNative not implemented for i386"
#elif defined(__aarch64__)
#warning "SetSigActToNative not implemented for aarch64"
#endif
}

void SetSigActToLinux(const SignalAction *native, k_sigaction *linux)
{
	linux->handler = native->sa_handler.Handler;
	linux->flags = native->Flags;
	linux->restorer = native->Restorer;

#if defined(__amd64__)
	unsigned long mask = native->Mask.to_ulong();
	mask = ConvertMaskToLinux(mask);

	linux->mask[0] = mask & 0xFFFFFFFF;
	linux->mask[1] = (uint32_t)((mask >> 32) & 0xFFFFFFFF);
	debug("m0:%#lx m1:%#lx | n:%#lx", linux->mask[0], linux->mask[1], native->Mask);
#elif defined(__i386__)
#warning "SetSigActToLinux not implemented for i386"
#elif defined(__aarch64__)
#warning "SetSigActToLinux not implemented for aarch64"
#endif
}

struct kstat KStatToStat(struct linux_kstat linux_stat)
{
	struct kstat stat;
	stat.Device = linux_stat.st_dev;
	stat.Index = linux_stat.st_ino;
	stat.HardLinks = (nlink_t)linux_stat.st_nlink;
	stat.Mode = linux_stat.st_mode;
	stat.UserID = linux_stat.st_uid;
	stat.GroupID = linux_stat.st_gid;
	stat.RawDevice = linux_stat.st_rdev;
	stat.Size = linux_stat.st_size;
	stat.BlockSize = linux_stat.st_blksize;
	stat.Blocks = linux_stat.st_blocks;
	stat.AccessTime = linux_stat.st_atime;
	// stat.st_atime_nsec = linux_stat.st_atime_nsec;
	stat.ModifyTime = linux_stat.st_mtime;
	// stat.st_mtime_nsec = linux_stat.st_mtime_nsec;
	stat.ChangeTime = linux_stat.st_ctime;
	// stat.st_ctime_nsec = linux_stat.st_ctime_nsec;
	return stat;
}

struct linux_kstat StatToKStat(struct kstat stat)
{
	struct linux_kstat linux_stat;
	linux_stat.st_dev = stat.Device;
	linux_stat.st_ino = stat.Index;
	linux_stat.st_nlink = stat.HardLinks;
	linux_stat.st_mode = stat.Mode;
	linux_stat.st_uid = stat.UserID;
	linux_stat.st_gid = stat.GroupID;
	linux_stat.st_rdev = stat.RawDevice;
	linux_stat.st_size = stat.Size;
	linux_stat.st_blksize = stat.BlockSize;
	linux_stat.st_blocks = stat.Blocks;
	linux_stat.st_atime = stat.AccessTime;
	// linux_stat.st_atime_nsec = stat.st_atime_nsec;
	linux_stat.st_mtime = stat.ModifyTime;
	// linux_stat.st_mtime_nsec = stat.st_mtime_nsec;
	linux_stat.st_ctime = stat.ChangeTime;
	// linux_stat.st_ctime_nsec = stat.st_ctime_nsec;
	return linux_stat;
}

struct kstat OKStatToStat(struct __old_kernel_stat okstat)
{
	struct kstat stat;
	stat.Device = okstat.st_dev;
	stat.Index = okstat.st_ino;
	stat.HardLinks = okstat.st_nlink;
	stat.Mode = okstat.st_mode;
	stat.UserID = okstat.st_uid;
	stat.GroupID = okstat.st_gid;
	stat.RawDevice = okstat.st_rdev;
	stat.Size = okstat.st_size;
	stat.AccessTime = okstat.st_atime;
	stat.ModifyTime = okstat.st_mtime;
	stat.ChangeTime = okstat.st_ctime;
	return stat;
}

struct __old_kernel_stat StatToOKStat(struct kstat stat)
{
	struct __old_kernel_stat okstat;
	okstat.st_dev = (unsigned short)stat.Device;
	okstat.st_ino = (unsigned short)stat.Index;
	okstat.st_nlink = (unsigned short)stat.HardLinks;
	okstat.st_mode = (unsigned short)stat.Mode;
	okstat.st_uid = (unsigned short)stat.UserID;
	okstat.st_gid = (unsigned short)stat.GroupID;
	okstat.st_rdev = (unsigned short)stat.RawDevice;
#ifdef __i386__
	okstat.st_size = (unsigned long)stat.Size;
	okstat.st_atime = (unsigned long)stat.AccessTime;
	okstat.st_mtime = (unsigned long)stat.ModifyTime;
	okstat.st_ctime = (unsigned long)stat.ChangeTime;
#else
	okstat.st_size = (unsigned int)stat.Size;
	okstat.st_atime = (unsigned int)stat.AccessTime;
	okstat.st_mtime = (unsigned int)stat.ModifyTime;
	okstat.st_ctime = (unsigned int)stat.ChangeTime;
#endif
	return okstat;
}

__no_stack_protector void __LinuxForkReturn(void *tableAddr)
{
#if defined(__amd64__)
	asmv("movq %0, %%cr3" ::"r"(tableAddr)); /* Load process page table */
	asmv("movq $0, %rax\n");				 /* Return 0 */
	asmv("movq %r8, %rsp\n");				 /* Restore stack pointer */
	asmv("movq %r8, %rbp\n");				 /* Restore base pointer */
	asmv("swapgs\n");						 /* Swap GS back to the user GS */
	asmv("sti\n");							 /* Enable interrupts */
	asmv("sysretq\n");						 /* Return to rcx address in user mode */
#elif defined(__i386__)
#warning "__LinuxForkReturn not implemented for i386"
#endif
	__builtin_unreachable();
}

static ssize_t linux_read(SysFrm *, int fd, void *buf, size_t count)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	void *pBuf = vma->UserCheckAndGetAddress(buf, count);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	func("%d, %p, %d", fd, buf, count);

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	ssize_t ret = ConvertErrnoToLinux(fdt->usr_read(fd, pBuf, count));
	if (ret >= 0)
		fdt->usr_lseek(fd, ret, SEEK_CUR);

#ifdef DEBUG
	DumpData("READ", pBuf, ret < 0 ? 0 : ret);
#endif
	return ret;
}

static ssize_t linux_write(SysFrm *, int fd, const void *buf, size_t count)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const void *pBuf = vma->UserCheckAndGetAddress(buf, count);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	func("%d, %p, %d", fd, buf, count);

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	ssize_t ret = ConvertErrnoToLinux(fdt->usr_write(fd, pBuf, count));
	if (ret)
		fdt->usr_lseek(fd, ret, SEEK_CUR);

#ifdef DEBUG
	DumpData("WRITE", (void *)pBuf, ret < 0 ? 0 : ret);
#endif
	return ret;
}

static int linux_open(SysFrm *sf, const char *pathname, int flags, mode_t mode)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const char *pPathname = vma->UserCheckAndGetAddress(pathname, PAGE_SIZE);
	if (pPathname == nullptr)
		return -linux_EFAULT;

	func("%s, %d, %d", pPathname, flags, mode);

	if (flags & 0200000 /* O_DIRECTORY */)
	{
		Node node = fs->Lookup(pcb->CWD, pPathname);
		if (node == nullptr)
		{
			debug("Couldn't find %s", pPathname);
			return -linux_ENOENT;
		}

		if (!node->IsDirectory())
		{
			debug("%s is not a directory", pPathname);
			return -linux_ENOTDIR;
		}
	}

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_open(pPathname, flags, mode));
}

static int linux_close(SysFrm *, int fd)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_close(fd));
}

/* stat, lstat and fstat uses __old_kernel_stat:
https://github.com/torvalds/linux/blob/bfa8f18691ed2e978e4dd51190569c434f93e268/fs/stat.c#L353
if __ARCH_WANT_OLD_STAT is defined
so what about __old_kernel_stat? when it is used? */

static int linux_stat(SysFrm *, const char *pathname, struct linux_kstat *statbuf)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pPathname = vma->UserCheckAndGetAddress(pathname, PAGE_SIZE);
	if (pPathname == nullptr)
		return -linux_EFAULT;

	auto pStatbuf = vma->UserCheckAndGetAddress(statbuf);
	if (pStatbuf == nullptr)
		return -linux_EFAULT;

	struct kstat nstat = KStatToStat(*pStatbuf);
	int ret = ConvertErrnoToLinux(fdt->usr_stat(pPathname, &nstat));
	*pStatbuf = StatToKStat(nstat);
	return ret;
}

static int linux_fstat(SysFrm *, int fd, struct linux_kstat *statbuf)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pStatbuf = vma->UserCheckAndGetAddress(statbuf);
	if (pStatbuf == nullptr)
		return -linux_EFAULT;

	struct kstat nstat = KStatToStat(*pStatbuf);
	int ret = ConvertErrnoToLinux(fdt->usr_fstat(fd, &nstat));
	*pStatbuf = StatToKStat(nstat);
	return ret;
}

static int linux_lstat(SysFrm *, const char *pathname, struct linux_kstat *statbuf)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pPathname = vma->UserCheckAndGetAddress(pathname, PAGE_SIZE);
	auto pStatbuf = vma->UserCheckAndGetAddress(statbuf);
	if (pPathname == nullptr || pStatbuf == nullptr)
		return -linux_EFAULT;

	struct kstat nstat = KStatToStat(*pStatbuf);
	int ret = ConvertErrnoToLinux(fdt->usr_lstat(pPathname, &nstat));
	*pStatbuf = StatToKStat(nstat);
	return ret;
}

static int linux_poll(SysFrm *, linux_pollfd *fds, nfds_t nfds, int timeout)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pFds = vma->UserCheckAndGetAddress(fds, nfds * sizeof(linux_pollfd));
	if (pFds == nullptr)
		return -linux_EFAULT;

	for (int i = 0; i < nfds; i++)
	{
		if (pFds[i].fd < 0)
			return -linux_EBADF;

		debug("poll[%zu].fd=%d .events=%d .revents=%d",
			  i, pFds[i].fd, pFds[i].events, pFds[i].revents);
		pFds[i].revents = 0;
	}

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	UNUSED(fdt);
	UNUSED(timeout);
	fixme("poll is stub! returning 1");
	return 1;
}

// #include "../syscalls.h"

static off_t linux_lseek(SysFrm *, int fd, off_t offset, int whence)
{
	static_assert(linux_SEEK_SET == SEEK_SET);
	static_assert(linux_SEEK_CUR == SEEK_CUR);
	static_assert(linux_SEEK_END == SEEK_END);

	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_lseek(fd, offset, whence));
}

static void *linux_mmap(SysFrm *, void *addr, size_t length, int prot,
						int flags, int fildes, off_t offset)
{
	if (length == 0)
		return (void *)-linux_EINVAL;

	bool p_None = prot & linux_PROT_NONE;
	bool p_Read = prot & linux_PROT_READ;
	bool p_Write = prot & linux_PROT_WRITE;
	bool p_Exec = prot & linux_PROT_EXEC;

	bool m_Shared = flags & linux_MAP_SHARED;
	bool m_Private = flags & linux_MAP_PRIVATE;
	bool m_Fixed = flags & linux_MAP_FIXED;
	bool m_Anon = flags & linux_MAP_ANONYMOUS;

	UNUSED(p_None);
	UNUSED(m_Anon);

	debug("None:%d Read:%d Write:%d Exec:%d",
		  p_None, p_Read, p_Write, p_Exec);

	debug("Shared:%d Private:%d Fixed:%d Anon:%d",
		  m_Shared, m_Private, m_Fixed, m_Anon);

	int unknownFlags = flags & ~(linux_MAP_SHARED | linux_MAP_PRIVATE |
								 linux_MAP_FIXED | linux_MAP_ANONYMOUS);
	if (unknownFlags)
	{
		/* We still have some flags missing afaik... */
		fixme("Unknown flags: %x", unknownFlags);
		/* FIXME: Continue? */
	}

	if (offset % PAGE_SIZE)
		return (void *)-linux_EINVAL;

	if (uintptr_t(addr) % PAGE_SIZE && m_Fixed)
		return (void *)-linux_EINVAL;

	if ((m_Shared && m_Private) ||
		(!m_Shared && !m_Private))
		return (void *)-linux_EINVAL;

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;
	if (fildes != -1 && !m_Anon)
	{
		fixme("File mapping not fully implemented");
		vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;

		auto _fd = fdt->FileMap.find(fildes);
		if (_fd == fdt->FileMap.end())
		{
			debug("Invalid file descriptor %d", fildes);
			return (void *)-linux_EBADF;
		}

		if (p_Read)
		{
			void *pBuf = vma->RequestPages(TO_PAGES(length));
			debug("created buffer at %#lx-%#lx",
				  pBuf, (uintptr_t)pBuf + length);

			uintptr_t mFlags = Memory::US;
			if (p_Write)
				mFlags |= Memory::RW;

			if (m_Fixed)
			{
				if (m_Shared)
					return (void *)-linux_ENOSYS;

				int mRet = vma->Map(addr, pBuf, length, mFlags);
				if (mRet < 0)
				{
					debug("Failed to map file: %s", strerror(mRet));
					return (void *)(uintptr_t)ConvertErrnoToLinux(mRet);
				}
				off_t oldOff = fdt->usr_lseek(fildes, 0, SEEK_CUR);
				fdt->usr_lseek(fildes, offset, SEEK_SET);

				ssize_t ret = fdt->usr_read(fildes, pBuf, length);
				fdt->usr_lseek(fildes, oldOff, SEEK_SET);

				if (ret < 0)
				{
					debug("Failed to read file");
					return (void *)ConvertErrnoToLinux(ret);
				}
				return addr;
			}
			else
			{
				int mRet = vma->Map(pBuf, pBuf, length, mFlags);
				if (mRet < 0)
				{
					debug("Failed to map file: %s", strerror(mRet));
					return (void *)(uintptr_t)ConvertErrnoToLinux(mRet);
				}
			}

			off_t oldOff = fdt->usr_lseek(fildes, 0, SEEK_CUR);
			fdt->usr_lseek(fildes, offset, SEEK_SET);

			ssize_t ret = fdt->usr_read(fildes, pBuf, length);

			fdt->usr_lseek(fildes, oldOff, SEEK_SET);

			if (ret < 0)
			{
				debug("Failed to read file");
				return (void *)ConvertErrnoToLinux(ret);
			}
			return pBuf;
		}

		debug("???");
		return (void *)-linux_ENOSYS;
	}

	void *ret = vma->CreateCoWRegion(addr, length,
									 p_Read, p_Write, p_Exec,
									 m_Fixed, m_Shared);
	debug("ret: %#lx", ret);
	return (void *)ret;
}
#undef __FENNIX_KERNEL_SYSCALLS_LIST_H__

static int linux_mprotect(SysFrm *, void *addr, size_t len, int prot)
{
	if (len == 0)
		return -linux_EINVAL;

	if (uintptr_t(addr) % PAGE_SIZE)
		return -linux_EINVAL;

	// bool p_None = prot & linux_PROT_NONE;
	bool p_Read = prot & linux_PROT_READ;
	bool p_Write = prot & linux_PROT_WRITE;
	// bool p_Exec = prot & linux_PROT_EXEC;

	PCB *pcb = thisProcess;
	Memory::Virtual vmm = Memory::Virtual(pcb->PageTable);

	for (uintptr_t i = uintptr_t(addr);
		 i < uintptr_t(addr) + len;
		 i += PAGE_SIZE)
	{
		if (unlikely(vmm.Check((void *)i, Memory::G)))
		{
			warn("%p is a global page", (void *)i);
			return -linux_ENOMEM;
		}

		Memory::PageTableEntry *pte = vmm.GetPTE(addr);
		if (pte == nullptr)
		{
			debug("Page %#lx is not mapped inside %#lx",
				  (void *)i, pcb->PageTable);
			fixme("Page %#lx is not mapped", (void *)i);
			continue;
			return -linux_ENOMEM;
		}

#if defined(__amd64__) || defined(__i386__)
		if (!pte->Present ||
			(!pte->UserSupervisor && p_Read) ||
			(!pte->ReadWrite && p_Write))
		{
			debug("Page %p is not mapped with the correct permissions",
				  (void *)i);
			return -linux_EACCES;
		}

		// pte->Present = !p_None;
		pte->UserSupervisor = p_Read;
		pte->ReadWrite = p_Write;
// pte->ExecuteDisable = p_Exec;
#else
		UNUSED(p_Read);
		UNUSED(p_Write);
#endif

		debug("Changed permissions of page %#lx to %s %s %s %s",
			  (void *)i,
			  (prot & linux_PROT_NONE) ? "None" : "",
			  p_Read ? "Read" : "",
			  p_Write ? "Write" : "",
			  (prot & linux_PROT_EXEC) ? "Exec" : "");

#if defined(__amd64__)
		CPU::x64::invlpg(addr);
#elif defined(__i386__)
		CPU::x32::invlpg(addr);
#elif defined(__aarch64__)
		asmv("dsb sy");
		asmv("tlbi vae1is, %0" : : "r"(addr) : "memory");
		asmv("dsb sy");
		asmv("isb");
#endif
	}

	return 0;
}

static int linux_munmap(SysFrm *, void *addr, size_t length)
{
	if (uintptr_t(addr) % PAGE_SIZE)
		return -linux_EINVAL;

	if (length == 0)
		return -linux_EINVAL;

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;
	vma->FreePages((void *)addr, TO_PAGES(length));
	return 0;
}

static void *linux_brk(SysFrm *, void *addr)
{
	PCB *pcb = thisProcess;
	void *ret = pcb->ProgramBreak->brk(addr);
	debug("brk(%#lx) = %#lx", addr, ret);
	return (void *)ConvertErrnoToLinux(ret);
}

static int linux_ioctl(SysFrm *, int fd, unsigned long request, void *argp)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	void *pArgp = nullptr;
	if (argp != nullptr)
	{
		pArgp = vma->UserCheckAndGetAddress(argp);
		if (pArgp == nullptr)
			return -linux_EFAULT;
	}

	int ret = ConvertErrnoToLinux(fdt->usr_ioctl(fd, request, pArgp));
	return ret;
}

static ssize_t linux_pread64(SysFrm *, int fd, void *buf, size_t count, off_t offset)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	void *pBuf = vma->UserCheckAndGetAddress(buf, count);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	off_t oldOff = fdt->usr_lseek(fd, 0, SEEK_CUR);
	fdt->usr_lseek(fd, offset, SEEK_SET);
	ssize_t ret = fdt->usr_read(fd, pBuf, count);
	fdt->usr_lseek(fd, oldOff, SEEK_SET);
	return ConvertErrnoToLinux(ret);
}

static ssize_t linux_pwrite64(SysFrm *, int fd, const void *buf, size_t count, off_t offset)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const void *pBuf = vma->UserCheckAndGetAddress(buf, count);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	off_t oldOff = fdt->usr_lseek(fd, 0, SEEK_CUR);
	fdt->usr_lseek(fd, offset, SEEK_SET);
	ssize_t ret = fdt->usr_write(fd, pBuf, count);
	fdt->usr_lseek(fd, oldOff, SEEK_SET);
	return ConvertErrnoToLinux(ret);
}

static ssize_t linux_readv(SysFrm *sf, int fildes, const struct iovec *iov, int iovcnt)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const struct iovec *pIov = vma->UserCheckAndGetAddress(iov, sizeof(struct iovec) * iovcnt);
	if (pIov == nullptr)
		return -linux_EFAULT;

	ssize_t Total = 0;
	for (int i = 0; i < iovcnt; i++)
	{
		debug("%d: iov[%d]: %p %d", fildes, i, pIov[i].iov_base, pIov[i].iov_len);

		if (!pIov[i].iov_base)
		{
			debug("invalid iov_base");
			return -linux_EFAULT;
		}

		if (pIov[i].iov_len == 0)
		{
			debug("invalid iov_len");
			continue;
		}

		ssize_t n = linux_read(sf, fildes, pIov[i].iov_base, pIov[i].iov_len);
		if (n < 0)
			return ConvertErrnoToLinux(n);
		debug("n: %d", n);

		Total += n;
		if (n < (ssize_t)pIov[i].iov_len)
		{
			debug("break");
			break;
		}
	}
	debug("readv: %d", Total);
	return Total;
}

static ssize_t linux_writev(SysFrm *sf, int fildes, const struct iovec *iov, int iovcnt)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const struct iovec *pIov = vma->UserCheckAndGetAddress(iov, sizeof(struct iovec) * iovcnt);
	if (pIov == nullptr)
		return -linux_EFAULT;

	ssize_t Total = 0;
	for (int i = 0; i < iovcnt; i++)
	{
		debug("%d: iov[%d]: %p %d", fildes, i, pIov[i].iov_base, pIov[i].iov_len);

		if (!pIov[i].iov_base)
		{
			debug("invalid iov_base");
			return -linux_EFAULT;
		}

		if (pIov[i].iov_len == 0)
		{
			debug("invalid iov_len");
			continue;
		}

		ssize_t n = linux_write(sf, fildes, pIov[i].iov_base, pIov[i].iov_len);
		if (n < 0)
			return ConvertErrnoToLinux(n);
		debug("n: %d", n);

		Total += n;
		if (n < (ssize_t)pIov[i].iov_len)
		{
			debug("break");
			break;
		}
	}
	debug("writev: %d", Total);
	return Total;
}

static int linux_access(SysFrm *, const char *pathname, int mode)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pPathname = vma->UserCheckAndGetAddress(pathname);
	if (pPathname == nullptr)
		return -linux_EFAULT;

	debug("access(%s, %d)", (char *)pPathname, mode);

	if (!fs->Lookup(pcb->CWD, pPathname))
		return -linux_ENOENT;

	stub;
	return 0;
}

static int linux_pipe(SysFrm *, int pipefd[2])
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	int *pPipefd = vma->UserCheckAndGetAddress(pipefd);
	debug("pipefd=%#lx", pPipefd);
	fixme("pipefd=[%d, %d]", pPipefd[0], pPipefd[1]);
	return -linux_ENOSYS;
}

static int linux_madvise(SysFrm *, void *addr, size_t length, int advice)
{
	// PCB *pcb = thisProcess;
	// Memory::VirtualMemoryArea *vma = pcb->vma;
	/* TODO: For now we ignore the advice and just return 0 */
	/* "man 2 madvise" for more info */
	stub;
	return 0;
}

static int linux_dup(SysFrm *, int oldfd)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_dup(oldfd));
}

static int linux_dup2(SysFrm *, int oldfd, int newfd)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_dup2(oldfd, newfd));
}

static int linux_pause(SysFrm *)
{
	PCB *pcb = thisProcess;
	return ConvertErrnoToLinux(pcb->Signals.WaitAnySignal());
}

static int linux_nanosleep(SysFrm *,
						   const struct timespec *req,
						   struct timespec *rem)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pReq = vma->UserCheckAndGetAddress(req);
	auto pRem = vma->UserCheckAndGetAddress(rem);
	if (pReq == nullptr || pRem == nullptr)
		return -linux_EFAULT;

	if (pReq->tv_nsec < 0 || pReq->tv_nsec > 999999999)
	{
		debug("Invalid tv_nsec %ld", pReq->tv_nsec);
		return -linux_EINVAL;
	}

	if (pReq->tv_sec < 0)
	{
		debug("Invalid tv_sec %ld", pReq->tv_sec);
		return -linux_EINVAL;
	}

	debug("tv_nsec=%ld tv_sec=%ld",
		  pReq->tv_nsec, pReq->tv_sec);

	uint64_t nanoTime = pReq->tv_nsec;
	uint64_t secTime = FromSeconds(pReq->tv_sec);

	uint64_t time = GlobalClock->Now();
	uint64_t sleepTime = GlobalClock->Now() + secTime + nanoTime;

	debug("time=%ld secTime=%ld nanoTime=%ld sleepTime=%ld",
		  time, secTime, nanoTime, sleepTime);

	while (time < sleepTime)
	{
		if (pcb->Signals.HasPendingSignal())
		{
			debug("sleep interrupted by signal");
			return -linux_EINTR;
		}

		pcb->GetContext()->Yield();
		time = GlobalClock->Now();
	}
	debug("time=     %ld", time);
	debug("sleepTime=%ld", sleepTime);

	if (rem)
	{
		pRem->tv_sec = 0;
		pRem->tv_nsec = 0;
	}
	return 0;
}

static pid_t linux_getpid(SysFrm *)
{
	PCB *pcb = thisProcess;
	return pcb->ID;
}

static int linux_setitimer(SysFrm *, int which,
						   const struct itimerspec64 *new_value,
						   struct itimerspec64 *old_value)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pNewValue = vma->UserCheckAndGetAddress(new_value);
	auto pOldValue = vma->UserCheckAndGetAddress(old_value);
	if (pNewValue == nullptr)
		return -linux_EFAULT;

	if (pOldValue == nullptr && old_value)
		return -linux_EFAULT;

	switch (which)
	{
	case linux_ITIMER_REAL:
	{
		fixme("ITIMER_REAL not implemented");
		return 0;
	}
	case linux_ITIMER_VIRTUAL:
	{
		fixme("ITIMER_VIRTUAL not implemented");
		return 0;
	}
	case linux_ITIMER_PROF:
	{
		fixme("ITIMER_PROF not implemented");
		return 0;
	}
	default:
		return -linux_EINVAL;
	}

	return 0;
}

static int linux_shutdown(SysFrm *, int sockfd, int how)
{
	stub;
	return -linux_ENOSYS;
}

static pid_t linux_fork(SysFrm *sf)
{
	TCB *Thread = thisThread;
	PCB *Parent = Thread->Parent;

	PCB *NewProcess =
		TaskManager->CreateProcess(Parent, Parent->Name,
								   Parent->Security.ExecutionMode,
								   true);
	if (unlikely(!NewProcess))
	{
		error("Failed to create process for fork");
		return -linux_EAGAIN;
	}

	NewProcess->Security.ProcessGroupID = Parent->Security.ProcessGroupID;
	NewProcess->Security.SessionID = Parent->Security.SessionID;

	NewProcess->PageTable = Parent->PageTable->Fork();
	NewProcess->vma->Table = NewProcess->PageTable;
	NewProcess->vma->Fork(Parent->vma);
	NewProcess->ProgramBreak->SetTable(NewProcess->PageTable);
	NewProcess->FileDescriptors->Fork(Parent->FileDescriptors);
	NewProcess->Executable = Parent->Executable;
	NewProcess->CWD = Parent->CWD;
	NewProcess->FileCreationMask = Parent->FileCreationMask;

	TCB *NewThread =
		TaskManager->CreateThread(NewProcess,
								  0,
								  nullptr,
								  nullptr,
								  std::vector<AuxiliaryVector>(),
								  Thread->Info.Architecture,
								  Thread->Info.Compatibility,
								  true);
	if (!NewThread)
	{
		error("Failed to create thread for fork");
		delete NewProcess;
		return -linux_EAGAIN;
	}
	NewThread->Rename(Thread->Name);

	TaskManager->UpdateFrame();

#if defined(__amd64__) || defined(__i386__)
	NewThread->FPU = Thread->FPU;
#endif
	NewThread->Stack->Fork(Thread->Stack);
	NewThread->Info.Architecture = Thread->Info.Architecture;
	NewThread->Info.Compatibility = Thread->Info.Compatibility;
	NewThread->Security.IsCritical = Thread->Security.IsCritical;
	NewThread->Registers = Thread->Registers;
#if defined(__amd64__)
	NewThread->Registers.rip = (uintptr_t)__LinuxForkReturn;
	/* For sysretq */
	NewThread->Registers.rdi = (uintptr_t)NewProcess->PageTable;
	NewThread->Registers.rcx = sf->ReturnAddress;
	NewThread->Registers.r8 = sf->StackPointer;
#else
#warning "sys_fork not implemented for other platforms"
#endif

#if defined(__amd64__) || defined(__i386__)
	NewThread->GSBase = NewThread->ShadowGSBase;
	NewThread->ShadowGSBase = Thread->ShadowGSBase;
	NewThread->FSBase = Thread->FSBase;
#endif

	debug("ret addr: %#lx, stack: %#lx ip: %#lx", sf->ReturnAddress,
		  sf->StackPointer, (uintptr_t)__LinuxForkReturn);
	debug("Forked thread \"%s\"(%d) to \"%s\"(%d)",
		  Thread->Name, Thread->ID,
		  NewThread->Name, NewThread->ID);
	NewThread->SetState(Tasking::Ready);

	// Parent->GetContext()->Yield();
	return (int)NewProcess->ID;
}

static pid_t linux_vfork(SysFrm *sf)
{
	TCB *Thread = thisThread;
	PCB *Parent = Thread->Parent;

	PCB *NewProcess =
		TaskManager->CreateProcess(Parent, Parent->Name,
								   Parent->Security.ExecutionMode,
								   true);
	if (unlikely(!NewProcess))
	{
		error("Failed to create process for vfork");
		return -linux_EAGAIN;
	}

	NewProcess->Security.ProcessGroupID = Parent->Security.ProcessGroupID;
	NewProcess->Security.SessionID = Parent->Security.SessionID;

	NewProcess->PageTable = Parent->PageTable;
	delete NewProcess->vma;
	NewProcess->vma = Parent->vma;
	delete NewProcess->ProgramBreak;
	NewProcess->ProgramBreak = NewProcess->ProgramBreak;
	NewProcess->FileDescriptors->Fork(Parent->FileDescriptors);
	NewProcess->Executable = Parent->Executable;
	NewProcess->CWD = Parent->CWD;
	NewProcess->FileCreationMask = Parent->FileCreationMask;

	TCB *NewThread =
		TaskManager->CreateThread(NewProcess,
								  0,
								  nullptr,
								  nullptr,
								  std::vector<AuxiliaryVector>(),
								  Thread->Info.Architecture,
								  Thread->Info.Compatibility,
								  true);
	if (!NewThread)
	{
		error("Failed to create thread for fork");
		delete NewProcess;
		return -linux_EAGAIN;
	}
	NewThread->Rename(Thread->Name);

	TaskManager->UpdateFrame();

#if defined(__amd64__) || defined(__i386__)
	NewThread->FPU = Thread->FPU;
#endif
	delete NewThread->Stack;
	NewThread->Stack = Thread->Stack;
	NewThread->Info.Architecture = Thread->Info.Architecture;
	NewThread->Info.Compatibility = Thread->Info.Compatibility;
	NewThread->Security.IsCritical = Thread->Security.IsCritical;
	NewThread->Registers = Thread->Registers;
#if defined(__amd64__)
	NewThread->Registers.rip = (uintptr_t)__LinuxForkReturn;
	/* For sysretq */
	NewThread->Registers.rdi = (uintptr_t)NewProcess->PageTable;
	NewThread->Registers.rcx = sf->ReturnAddress;
	NewThread->Registers.r8 = sf->StackPointer;
#else
#warning "sys_fork not implemented for other platforms"
#endif

#if defined(__amd64__) || defined(__i386__)
	NewThread->GSBase = NewThread->ShadowGSBase;
	NewThread->ShadowGSBase = Thread->ShadowGSBase;
	NewThread->FSBase = Thread->FSBase;
#endif

	debug("ret addr: %#lx, stack: %#lx ip: %#lx", sf->ReturnAddress,
		  sf->StackPointer, (uintptr_t)__LinuxForkReturn);
	debug("Forked thread \"%s\"(%d) to \"%s\"(%d)",
		  Thread->Name, Thread->ID,
		  NewThread->Name, NewThread->ID);

	{
		CriticalSection cs;
		Thread->SetState(Tasking::Frozen);
		NewProcess->Linux.vforked = true;
		NewProcess->Linux.CallingThread = Thread;
		NewThread->SetState(Tasking::Ready);
	}
	Parent->GetContext()->Yield();
	return (int)NewProcess->ID;
}

__no_sanitize("undefined") static int linux_execve(SysFrm *sf, const char *pathname, char *const argv[], char *const envp[])
{
	/* FIXME: exec doesn't follow the UNIX standard
		The pid, open files, etc. should be preserved */
	TCB *tcb = thisThread;
	PCB *pcb = tcb->Parent;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pPathname = vma->UserCheckAndGetAddress(pathname, PAGE_SIZE);
	auto pArgv = vma->UserCheckAndGetAddress(argv, 1 /*MAX_ARG*/); /* MAX_ARG is too much? */
	auto pEnvp = vma->UserCheckAndGetAddress(envp, 1 /*MAX_ARG*/);
	if (pPathname == nullptr || pArgv == nullptr || pEnvp == nullptr)
		return -linux_EFAULT;

	func("%s %#lx %#lx", pPathname, pArgv, pEnvp);

	int argvLen = 0;
	for (argvLen = 0; MAX_ARG; argvLen++)
	{
		if (vma->UserCheck(pArgv[argvLen]) < 0)
			break;

		auto arg = pcb->PageTable->Get(pArgv[argvLen]);
		if (arg == nullptr)
			break;
	}

	int envpLen = 0;
	for (envpLen = 0; MAX_ARG; envpLen++)
	{
		if (vma->UserCheck(pEnvp[envpLen]) < 0)
			break;

		auto arg = pcb->PageTable->Get(pEnvp[envpLen]);
		if (arg == nullptr)
			break;
	}

	char **safeArgv = (char **)pcb->vma->RequestPages(TO_PAGES(argvLen * sizeof(char *)));
	char **safeEnvp = (char **)pcb->vma->RequestPages(TO_PAGES(envpLen * sizeof(char *)));

	const char *arg;
	char *nArg;
	for (int i = 0; i < argvLen; i++)
	{
		arg = pcb->PageTable->Get(pArgv[i]);
		assert(arg != nullptr);
		size_t len = strlen(arg);
		debug("arg[%d]: %s", i, arg);

		nArg = (char *)pcb->vma->RequestPages(TO_PAGES(len));
		memcpy((void *)nArg, arg, len);
		nArg[len] = '\0';

		safeArgv[i] = nArg;

		if (likely(i < MAX_ARG - 1))
			safeArgv[i + 1] = nullptr;
	}

	for (int i = 0; i < envpLen; i++)
	{
		arg = pcb->PageTable->Get(pEnvp[i]);
		assert(arg != nullptr);
		size_t len = strlen(arg);
		debug("env[%d]: %s", i, arg);

		nArg = (char *)pcb->vma->RequestPages(TO_PAGES(len));
		memcpy((void *)nArg, arg, len);
		nArg[len] = '\0';

		safeEnvp[i] = nArg;

		if (likely(i < MAX_ARG - 1))
			safeEnvp[i + 1] = nullptr;
	}

	Node file = fs->Lookup(pcb->CWD, pPathname);

	if (!file)
	{
		error("File not found");
		return -linux_ENOENT;
	}

	char shebangMagic[2]{};
	fs->Read(file, (uint8_t *)shebangMagic, 2, 0);

	if (shebangMagic[0] == '#' && shebangMagic[1] == '!')
	{
		char *origPath = (char *)pcb->vma->RequestPages(TO_PAGES(strlen(pPathname) + 1));
		memcpy(origPath, pPathname, strlen(pPathname) + 1);

		char *shebang = (char *)pPathname;
		size_t shebangLength = 0;
		constexpr int shebangLengthMax = 255;
		off_t shebangOffset = 2;
		while (true)
		{
			char c;
			if (fs->Read(file, (uint8_t *)&c, 1, shebangOffset) == 0)
				break;
			if (c == '\n' || shebangLength == shebangLengthMax)
				break;
			shebang[shebangLength++] = c;
			shebangOffset++;
		}
		shebang[shebangLength] = '\0';
		debug("Shebang: %s", shebang);

		char **cSafeArgv = (char **)pcb->vma->RequestPages(TO_PAGES(MAX_ARG));
		int i = 0;
		for (; safeArgv[i] != nullptr; i++)
		{
			size_t argLength = strlen(safeArgv[i]);
			char *cArg = (char *)pcb->vma->RequestPages(TO_PAGES(argLength));
			memcpy((void *)cArg, safeArgv[i], argLength);
			cArg[argLength] = '\0';

			cSafeArgv[i] = cArg;
			debug("cSafeArgv[%d]: %s", i, cSafeArgv[i]);
		}
		cSafeArgv[i] = nullptr;

		char *token = strtok(shebang, " ");
		i = 0;
		while (token != nullptr)
		{
			size_t len = strlen(token);
			char *t_arg = (char *)pcb->vma->RequestPages(TO_PAGES(len));
			memcpy((void *)t_arg, token, len);
			t_arg[len] = '\0';

			safeArgv[i++] = t_arg;
			token = strtok(nullptr, " ");
		}

		safeArgv[i++] = origPath;
		for (int j = 1; cSafeArgv[j] != nullptr; j++)
		{
			safeArgv[i++] = cSafeArgv[j];
			debug("clone: safeArgv[%d]: %s",
				  i, safeArgv[i - 1]);
		}
		safeArgv[i] = nullptr;

		debug("calling linux_execve with %s", safeArgv[0]);

		PCB *newPcb = TaskManager->CreateProcess(pcb, safeArgv[0], Tasking::TaskExecutionMode::User, false, pcb->Security.Real.UserID, pcb->Security.Real.GroupID);
		if (!newPcb)
		{
			error("Failed to create process for interpreter");
			return -linux_EAGAIN;
		}

		newPcb->Security = pcb->Security;
		newPcb->Info = pcb->Info;
		newPcb->FileDescriptors = pcb->FileDescriptors;
		newPcb->CWD = pcb->CWD;
		newPcb->PageTable = pcb->PageTable;
		newPcb->vma = pcb->vma;
		newPcb->ProgramBreak = pcb->ProgramBreak;

		char **newArgv = (char **)newPcb->vma->RequestPages(TO_PAGES(i * sizeof(char *)));
		char **newEnvp = (char **)newPcb->vma->RequestPages(TO_PAGES(envpLen * sizeof(char *)));

		for (int j = 0; j < i; j++)
		{
			size_t len = strlen(safeArgv[j]);
			char *newArg = (char *)newPcb->vma->RequestPages(TO_PAGES(len));
			memcpy(newArg, safeArgv[j], len);
			newArg[len] = '\0';
			newArgv[j] = newArg;
		}
		newArgv[i] = nullptr;

		for (int j = 0; j < envpLen; j++)
		{
			size_t len = strlen(safeEnvp[j]);
			char *newEnv = (char *)newPcb->vma->RequestPages(TO_PAGES(len));
			memcpy(newEnv, safeEnvp[j], len);
			newEnv[len] = '\0';
			newEnvp[j] = newEnv;
		}
		newEnvp[envpLen] = nullptr;

		int ret = Execute::Spawn((char *)safeArgv[0], (const char **)newArgv, (const char **)newEnvp,
								 newPcb, true, newPcb->Info.Compatibility);

		if (ret < 0)
		{
			error("Failed to spawn interpreter");
			return ConvertErrnoToLinux(ret);
		}

		GetCurrentCPU()->CurrentProcess = newPcb;
		GetCurrentCPU()->CurrentThread = newPcb->Threads[0];

		while (true)
			newPcb->GetContext()->Yield();
		__builtin_unreachable();
	}

	if (pcb->Linux.vforked)
	{
		debug("vforked: %s", pPathname);
		CriticalSection cs;

		pcb->Linux.CallingThread->SetState(Tasking::Ready);
		pcb->Linux.vforked = false;

		pcb->PageTable = KernelPageTable->Fork();
		pcb->vma = new Memory::VirtualMemoryArea(pcb->PageTable);
		pcb->ProgramBreak = new Memory::ProgramBreak(pcb->PageTable, pcb->vma);
	}

	debug("spawn(%s %#lx %#lx %#lx %d %d)", pPathname, safeArgv, safeEnvp, pcb, true, pcb->Info.Compatibility);
	int ret = Execute::Spawn((char *)pPathname, (const char **)safeArgv, (const char **)safeEnvp,
							 pcb, true, pcb->Info.Compatibility);

	if (ret < 0)
	{
		error("Failed to spawn");
		return ConvertErrnoToLinux(ret);
	}

	const char *baseName;
	cwk_path_get_basename(pPathname, &baseName, nullptr);

	pcb->Rename(baseName);
	pcb->SetWorkingDirectory(fs->Convert(file->Parent));
	pcb->SetExe(pPathname);

	Tasking::Task *ctx = pcb->GetContext();
	// ctx->Sleep(1000);
	// pcb->SetState(Tasking::Zombie);
	// pcb->SetExitCode(0); /* FIXME: get process exit code */
	while (true)
		ctx->Yield();
	__builtin_unreachable();
}

static __noreturn void linux_exit(SysFrm *, int status)
{
	TCB *t = thisThread;
	{
		CriticalSection cs;
		trace("Userspace thread %s(%d) exited with code %d (%#x)",
			  t->Name,
			  t->ID, status,
			  status < 0 ? -status : status);

		t->SetState(Tasking::Zombie);
		t->SetExitCode(status);
		if (t->Parent->Linux.vforked)
			t->Parent->Linux.CallingThread->SetState(Tasking::Ready);
	}
	while (true)
		t->GetContext()->Yield();
	__builtin_unreachable();
}

static pid_t linux_wait4(SysFrm *, pid_t pid, int *wstatus,
						 int options, struct rusage *rusage)
{
	static_assert(sizeof(struct rusage) < PAGE_SIZE);
	/* FIXME: this function is very poorly implemented, the way
		it handles the zombie & coredump processes is very
		inefficient and should be rewritten */

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	if (pid == -1)
	{
		if (pcb->Children.empty())
		{
			debug("No children");
			return -linux_ECHILD;
		}

		std::vector<PCB *> wChilds;
		for (auto child : pcb->Children)
		{
			if (child->State == Tasking::Zombie)
			{
				if (wstatus != nullptr)
				{
					int *pWstatus = pcb->PageTable->Get(wstatus);
					*pWstatus = 0;

					bool ProcessExited = true;
					int ExitStatus = child->ExitCode.load();

					debug("Process returned %d", ExitStatus);

					if (ProcessExited)
						*pWstatus |= ExitStatus << 8;
				}

				if (rusage != nullptr)
				{
					size_t kTime = child->Info.KernelTime;
					size_t uTime = child->Info.UserTime;
					size_t _maxrss = child->GetSize();

					struct rusage *pRusage = vma->UserCheckAndGetAddress(rusage);
					if (pRusage == nullptr)
						return -linux_EFAULT;

					pRusage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
					pRusage->ru_utime.tv_usec = uTime / 1000000000;		 /* Microseconds */

					pRusage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
					pRusage->ru_stime.tv_usec = kTime / 1000000000;		 /* Microseconds */

					pRusage->ru_maxrss = _maxrss;
				}

				child->SetState(Tasking::Terminated);
				return child->ID;
			}

			if (child->State == Tasking::CoreDump)
			{
				if (wstatus != nullptr)
				{
					int *pWstatus = vma->UserCheckAndGetAddress(wstatus);
					if (pWstatus == nullptr)
						return -linux_EFAULT;
					*pWstatus = 0;

					bool ProcessExited = true;
					int ExitStatus = child->ExitCode.load();
					bool ProcessSignaled = true;
					bool CoreDumped = true;
					int TermSignal = child->Signals.GetLastSignal();
					TermSignal = ConvertSignalToLinux((signal_t)TermSignal);
					assert(TermSignal != SIGNULL);

					debug("Process returned %d", ExitStatus);

					if (ProcessExited)
						*pWstatus |= ExitStatus << 8;

					if (ProcessSignaled)
						*pWstatus |= TermSignal;

					if (CoreDumped)
						*pWstatus |= 0x80;
				}

				if (rusage != nullptr)
				{
					size_t kTime = child->Info.KernelTime;
					size_t uTime = child->Info.UserTime;
					size_t _maxrss = child->GetSize();

					struct rusage *pRusage = vma->UserCheckAndGetAddress(rusage);
					if (pRusage == nullptr)
						return -linux_EFAULT;

					pRusage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
					pRusage->ru_utime.tv_usec = uTime / 1000000000;		 /* Microseconds */

					pRusage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
					pRusage->ru_stime.tv_usec = kTime / 1000000000;		 /* Microseconds */

					pRusage->ru_maxrss = _maxrss;
				}

				child->SetState(Tasking::Terminated);
				return child->ID;
			}

			if (child->State != Tasking::Terminated)
				wChilds.push_back(child);
		}

		if (wChilds.empty())
		{
			debug("No children");
			return -linux_ECHILD;
		}

		fixme("Waiting for %d children", wChilds.size());
		pid = wChilds.front()->ID;
	}

	if (pid == 0)
	{
		fixme("Waiting for any child process whose process group ID is equal to that of the calling process");
		return -linux_ENOSYS;
	}

	if (pid < -1)
	{
		fixme("Waiting for any child process whose process group ID is equal to the absolute value of pid");
		return -linux_ENOSYS;
	}

	/* Wait for a child process, or any process? */
	PCB *tPcb = pcb->GetContext()->GetProcessByID(pid);
	if (!tPcb)
	{
		warn("Invalid PID %d", pid);
		return -linux_ECHILD;
	}

	if (options)
	{
#define WCONTINUED 1
#define WNOHANG 2
#define WUNTRACED 4
#define WEXITED 8
#define WNOWAIT 16
#define WSTOPPED 32
		fixme("options=%#x", options);
	}

#ifdef DEBUG
	for (auto child : pcb->Children)
		debug("Child: %s(%d)", child->Name, child->ID);
#endif

	debug("Waiting for %d(%#lx) state %d", pid, tPcb, tPcb->State);
	while (tPcb->State != Tasking::Zombie &&
		   tPcb->State != Tasking::CoreDump &&
		   tPcb->State != Tasking::Terminated)
		pcb->GetContext()->Yield();
	debug("Waited for %d(%#lx) state %d", pid, tPcb, tPcb->State);

	if (wstatus != nullptr)
	{
		int *pWstatus = vma->UserCheckAndGetAddress(wstatus);
		if (pWstatus == nullptr)
			return -linux_EFAULT;
		*pWstatus = 0;

		bool ProcessExited = true;
		int ExitStatus = tPcb->ExitCode.load();
		bool ProcessSignaled = false;
		bool CoreDumped = false;
		bool ProcessStopped = false;
		bool ProcessContinued = false;
		int TermSignal = 0;

		debug("Process returned %d", ExitStatus);

		if (ProcessExited)
			*pWstatus |= ExitStatus << 8;

		if (ProcessSignaled)
			*pWstatus |= TermSignal;

		if (CoreDumped)
			*pWstatus |= 0x80;

		/* FIXME: Untested */

		if (ProcessStopped)
			*pWstatus |= 0x7F;

		if (ProcessContinued)
			*pWstatus = 0xFFFF;

		debug("wstatus=%#x", *pWstatus);
	}

	if (rusage != nullptr)
	{
		size_t kTime = tPcb->Info.KernelTime;
		size_t uTime = tPcb->Info.UserTime;
		size_t _maxrss = tPcb->GetSize();

		struct rusage *pRusage = vma->UserCheckAndGetAddress(rusage);
		if (pRusage == nullptr)
			return -linux_EFAULT;

		pRusage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
		pRusage->ru_utime.tv_usec = uTime / 1000000000;		 /* Microseconds */

		pRusage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
		pRusage->ru_stime.tv_usec = kTime / 1000000000;		 /* Microseconds */

		pRusage->ru_maxrss = _maxrss;
		/* TODO: The rest of the fields */
	}

	return pid;
}

static int linux_kill(SysFrm *, pid_t pid, int sig)
{
	PCB *pcb = thisProcess->GetContext()->GetProcessByID(pid);
	if (!pcb)
		return -linux_ESRCH;

	/* TODO: Check permissions */

	if (sig == 0)
		return 0;

	if (pid == 0)
	{
		bool found = false;
		signal_t nSig = ConvertSignalToNative(sig);
		assert(nSig != SIGNULL);
		for (auto proc : pcb->GetContext()->GetProcessList())
		{
			if (proc->Security.ProcessGroupID == thisProcess->Security.ProcessGroupID)
			{
				debug("Sending signal %s to %s(%d)", lSigStr[sig], proc->Name, proc->ID);
				proc->SendSignal(nSig);
				found = true;
			}
		}
		if (!found)
			return -linux_ESRCH;
		return 0;
	}

	if (pid == -1)
	{
		fixme("Sending signal %d to all processes except init", sig);
		return -linux_ENOSYS;
	}

	if (pid < -1)
	{
		fixme("Sending signal %d to process group %d", sig, pid);
		return -linux_ENOSYS;
	}

	signal_t nSig = ConvertSignalToNative(sig);
	assert(nSig != SIGNULL);
	return ConvertErrnoToLinux(pcb->SendSignal(nSig));
}

static int linux_uname(SysFrm *, struct utsname *buf)
{
	assert(sizeof(struct utsname) < PAGE_SIZE);

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pBuf = vma->UserCheckAndGetAddress(buf);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	struct utsname uname =
		{
			/* TODO: This shouldn't be hardcoded */
			.sysname = KERNEL_NAME,
			.nodename = "fennix",
			.release = KERNEL_VERSION,
			.version = KERNEL_VERSION,
#if defined(__amd64__)
			.machine = "x86_64",
#elif defined(__i386__)
			.machine = "i386",
#elif defined(__aarch64__)
			.machine = "arm64",
#elif defined(aa32)
			.machine = "arm",
#endif
	};

	Node root = fs->GetRoot(0);
	Node rn = fs->Lookup(root, "/etc/subsystem/linux");
	if (rn)
	{
		struct kstat st;
		fs->Stat(rn, &st);

		char *sh = new char[st.Size];
		fs->Read(rn, sh, st.Size, 0);

		ini_t *ini = ini_load(sh, NULL);
		int section = ini_find_section(ini, "uname", NULL);
		int sysIdx = ini_find_property(ini, section, "sysname", NULL);
		int nodIdx = ini_find_property(ini, section, "nodename", NULL);
		int relIdx = ini_find_property(ini, section, "release", NULL);
		int verIdx = ini_find_property(ini, section, "version", NULL);
		int macIdx = ini_find_property(ini, section, "machine", NULL);
		const char *uSys = ini_property_value(ini, section, sysIdx);
		const char *uNod = ini_property_value(ini, section, nodIdx);
		const char *uRel = ini_property_value(ini, section, relIdx);
		const char *uVer = ini_property_value(ini, section, verIdx);
		const char *uMac = ini_property_value(ini, section, macIdx);
		debug("sysname=%s", uSys);
		debug("nodename=%s", uNod);
		debug("release=%s", uRel);
		debug("version=%s", uVer);
		debug("machine=%s", uMac);

		if (uSys && strcmp(uSys, "auto") != 0)
			strncpy(uname.sysname, uSys, sizeof(uname.sysname));
		if (uNod && strcmp(uNod, "auto") != 0)
			strncpy(uname.nodename, uNod, sizeof(uname.nodename));
		if (uRel && strcmp(uRel, "auto") != 0)
			strncpy(uname.release, uRel, sizeof(uname.release));
		if (uVer && strcmp(uVer, "auto") != 0)
			strncpy(uname.version, uVer, sizeof(uname.version));
		if (uMac && strcmp(uMac, "auto") != 0)
			strncpy(uname.machine, uMac, sizeof(uname.machine));
		ini_destroy(ini);

		delete[] sh;
	}
	else
		warn("Couldn't open /etc/subsystem/linux");

	memcpy(pBuf, &uname, sizeof(struct utsname));
	return 0;
}

static int linux_fcntl(SysFrm *, int fd, int cmd, void *arg)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;

	switch (cmd)
	{
	case linux_F_DUPFD:
		return ConvertErrnoToLinux(fdt->usr_dup2(fd, static_cast<int>((uintptr_t)arg)));
	case linux_F_GETFD:
		return ConvertErrnoToLinux(fdt->GetFlags(fd));
	case linux_F_SETFD:
		return ConvertErrnoToLinux(fdt->SetFlags(fd, static_cast<int>((uintptr_t)arg)));
	case linux_F_GETFL:
	{
		fixme("F_GETFL is stub?");
		return ConvertErrnoToLinux(fdt->GetFlags(fd));
	}
	case linux_F_SETFL:
	{
		fixme("F_SETFL is stub?");
		int flags = static_cast<int>((uintptr_t)arg);
		if (flags & O_APPEND)
			fdt->SetFlags(fd, fdt->GetFlags(fd) | O_APPEND);
		else
			fdt->SetFlags(fd, fdt->GetFlags(fd) & ~O_APPEND);
		return 0;
	}
	case linux_F_DUPFD_CLOEXEC:
	{
		int ret = fdt->usr_dup2(fd, static_cast<int>((uintptr_t)arg));
		if (ret < 0)
			return ConvertErrnoToLinux(ret);

		auto it = fdt->FileMap.find(fd);
		if (it == fdt->FileMap.end())
			ReturnLogError(-linux_EBADF, "Invalid fd %d", fd);

		it->second.Flags |= linux_FD_CLOEXEC;
		return ret;
	}
	case linux_F_SETOWN:
	case linux_F_GETOWN:
	case linux_F_SETSIG:
	case linux_F_GETSIG:
	case linux_F_GETLK:
	case linux_F_SETLK:
	case linux_F_SETLKW:
	case linux_F_SETOWN_EX:
	case linux_F_GETOWN_EX:
	case linux_F_GETOWNER_UIDS:
	case linux_F_OFD_GETLK:
	case linux_F_OFD_SETLK:
	case linux_F_OFD_SETLKW:
	{
		fixme("cmd %d not implemented", cmd);
		return -linux_ENOSYS;
	}
	default:
	{
		debug("Invalid cmd %#x", cmd);
		return -linux_EINVAL;
	}
	}
}

static int linux_creat(SysFrm *, const char *pathname, mode_t mode)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	return ConvertErrnoToLinux(fdt->usr_creat(pathname, mode));
}

static long linux_getcwd(SysFrm *, char *buf, size_t size)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	char *pBuf = vma->UserCheckAndGetAddress(buf, size);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	std::string cwd = pcb->CWD->GetPath();
	if (cwd.length() >= size)
	{
		warn("Buffer too small (%ld < %ld)", cwd.length(), size);
		return -linux_ERANGE;
	}

	strncpy(pBuf, cwd.c_str(), cwd.length());
	pBuf[cwd.length()] = '\0';
	debug("cwd: \"%s\" with %ld bytes", cwd.c_str(), cwd.length());
	return cwd.length();
}

static int linux_chdir(SysFrm *, const char *path)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const char *pPath = vma->UserCheckAndGetAddress(path);
	if (!pPath)
		return -linux_EFAULT;

	Node n = fs->Lookup(pcb->CWD, pPath);
	if (!n)
		return -linux_ENOENT;

	pcb->SetWorkingDirectory(n);
	debug("Changed cwd to \"%s\"", n->GetPath().c_str());
	return 0;
}

static int linux_fchdir(SysFrm *, int fd)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;

	auto it = fdt->FileMap.find(fd);
	if (it == fdt->FileMap.end())
		return -linux_EBADF;

	pcb->SetWorkingDirectory(it->second.node);
	debug("Changed cwd to \"%s\"", it->second.node->GetPath().c_str());
	return 0;
}

static int linux_mkdir(SysFrm *, const char *pathname, mode_t mode)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;
	fixme("semi-stub");

	const char *pPathname = vma->UserCheckAndGetAddress(pathname);
	if (!pPathname)
		return -linux_EFAULT;

	mode &= ~pcb->FileCreationMask & 0777;

	Node n = fs->Create(pcb->CWD, pPathname, mode);
	if (!n)
		return -linux_EEXIST;
	return 0;
}

static ssize_t linux_readlink(SysFrm *, const char *pathname,
							  char *buf, size_t bufsiz)
{
	if (!pathname || !buf)
		return -linux_EINVAL;

	if (bufsiz > PAGE_SIZE)
	{
		warn("bufsiz is too large: %ld", bufsiz);
		return -linux_EINVAL;
	}

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const char *pPath = vma->UserCheckAndGetAddress(pathname);
	char *pBuf = vma->UserCheckAndGetAddress(buf);
	if (pPath == nullptr || pBuf == nullptr)
		return -linux_EFAULT;

	func("%s %#lx %ld", pPath, buf, bufsiz);
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	int fd = fdt->usr_open(pPath, O_RDONLY, 0);
	if (fd < 0)
		return -linux_ENOENT;

	auto it = fdt->FileMap.find(fd);
	if (it == fdt->FileMap.end())
		ReturnLogError(-linux_EBADF, "Invalid fd %d", fd);

	vfs::FileDescriptorTable::Fildes &fildes = it->second;
	Node node = fildes.node;
	fdt->usr_close(fd);

	if (!node->IsSymbolicLink())
		return -linux_EINVAL;

	return ConvertErrnoToLinux(fs->ReadLink(node, pBuf, bufsiz));
}

static int linux_fchmod(SysFrm *, int fd, mode_t mode)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;

	if (fdt->FileMap.find(fd) == fdt->FileMap.end())
		return -linux_EBADF;

	struct kstat stat;
	int ret = fdt->usr_fstat(fd, &stat);
	if (ret < 0)
		return ret;

	if (stat.UserID != pcb->Security.Effective.UserID)
		return -linux_EPERM;

	/* TODO: check if FS is read-only: -linux_EROFS */

	mode_t current = pcb->FileDescriptors->FileMap[fd].Mode;
	mode_t newMode = (current & ~07777) | (mode & 07777);

	/* TODO: add CAP_FSETID check */
	if (stat.GroupID != pcb->Security.Effective.GroupID)
		newMode &= ~S_ISGID;

	/* FIXME: actually write to FS; maybe with fdt->usr_chmod or similar? */
	pcb->FileDescriptors->FileMap[fd].Mode = newMode;
	return 0;
}

static int linux_chmod(SysFrm *sf, const char *pathname, mode_t mode)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const char *pPathname = vma->UserCheckAndGetAddress(pathname);
	if (pPathname == nullptr)
		return -linux_EFAULT;

	Node node = fs->Lookup(pcb->CWD, pPathname);
	if (!node)
		return -linux_ENOENT;

	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	int fd = fdt->usr_open(pPathname, O_RDONLY, 0);
	int ret = linux_fchmod(sf, fd, mode);
	fdt->usr_close(fd);
	return ret;
}

static mode_t linux_umask(SysFrm *, mode_t mask)
{
	PCB *pcb = thisProcess;
	mode_t old = pcb->FileCreationMask;
	pcb->FileCreationMask = mask & 0777;
	debug("old=%#o new=%#o (mask: %#lx)", old, pcb->FileCreationMask, mask);
	return old;
}

static int linux_getrusage(SysFrm *, int who, struct rusage *usage)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pUsage = vma->UserCheckAndGetAddress(usage);
	if (pUsage == nullptr)
		return -linux_EFAULT;

	switch (who)
	{
	case linux_RUSAGE_SELF:
	{
		size_t kTime = pcb->Info.KernelTime;
		size_t uTime = pcb->Info.UserTime;
		size_t _maxrss = pcb->GetSize();

		pUsage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
		pUsage->ru_utime.tv_usec = uTime / 1000000000;		/* Microseconds */

		pUsage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
		pUsage->ru_stime.tv_usec = kTime / 1000000000;		/* Microseconds */

		pUsage->ru_maxrss = _maxrss;
		break;
	}
	case linux_RUSAGE_CHILDREN:
	{
		size_t kTime = 0;
		size_t uTime = 0;
		size_t _maxrss = 0;

		for (auto child : pcb->Children)
		{
			kTime += child->Info.KernelTime;
			uTime += child->Info.UserTime;
			_maxrss += child->GetSize();
		}

		pUsage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
		pUsage->ru_utime.tv_usec = uTime / 1000000000;		/* Microseconds */

		pUsage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
		pUsage->ru_stime.tv_usec = kTime / 1000000000;		/* Microseconds */

		pUsage->ru_maxrss = _maxrss;
		break;
	}
	case linux_RUSAGE_THREAD:
	{
		TCB *tcb = thisThread;

		size_t kTime = tcb->Info.KernelTime;
		size_t uTime = tcb->Info.UserTime;
		size_t _maxrss = tcb->GetSize();

		pUsage->ru_utime.tv_sec = uTime / 1000000000000000; /* Seconds */
		pUsage->ru_utime.tv_usec = uTime / 1000000000;		/* Microseconds */

		pUsage->ru_stime.tv_sec = kTime / 1000000000000000; /* Seconds */
		pUsage->ru_stime.tv_usec = kTime / 1000000000;		/* Microseconds */

		pUsage->ru_maxrss = _maxrss;
		break;
	}
	default:
		return -linux_EINVAL;
	}

	return 0;
}

static int linux_sysinfo(SysFrm *, struct sysinfo *info)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pInfo = vma->UserCheckAndGetAddress(info);
	if (pInfo == nullptr)
		return -linux_EFAULT;

	uint64_t nano = GlobalClock->Now();
	if (nano != 0)
		nano /= 10000000;

	pInfo->uptime = nano;
	pInfo->loads[0] = 0;
	pInfo->loads[1] = 0;
	pInfo->loads[2] = 0;
	pInfo->totalram = KernelAllocator.GetTotalMemory() - KernelAllocator.GetReservedMemory();
	pInfo->freeram = KernelAllocator.GetFreeMemory();
	pInfo->sharedram = 0;
	pInfo->bufferram = 0;
	pInfo->totalswap = 0;
	pInfo->freeswap = 0;
	pInfo->procs = TaskManager->GetProcessList().size();
	pInfo->totalhigh = 0;
	pInfo->freehigh = 0;
	pInfo->mem_unit = 1;
	if (sizeof(pInfo->_f) != 0)
		memset(pInfo->_f, 0, sizeof(pInfo->_f));
	return 0;
}

static int linux_syslog(SysFrm *, int type, char *bufp, int size)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	auto pbufp = vma->UserCheckAndGetAddress(bufp);
	if (pbufp == nullptr)
		return -linux_EFAULT;

	switch (type)
	{
	case linux_SYSLOG_ACTION_CLOSE:
		/* NOP */
		return 0;
	case linux_SYSLOG_ACTION_OPEN:
		/* NOP */
		return 0;
	case linux_SYSLOG_ACTION_READ:
	{
		fixme("SYSLOG_ACTION_READ not implemented");
		const char dummy[12] = "stub string";
		memcpy(pbufp, dummy, sizeof(dummy));
		return sizeof(dummy);
	}
	case linux_SYSLOG_ACTION_READ_ALL:
	{
		fixme("SYSLOG_ACTION_READ_ALL not implemented");
		const char dummy[12] = "stub string";
		memcpy(pbufp, dummy, sizeof(dummy));
		return sizeof(dummy);
	}
	case linux_SYSLOG_ACTION_READ_CLEAR:
	{
		fixme("SYSLOG_ACTION_READ_CLEAR not implemented");
		const char dummy[12] = "stub string";
		memcpy(pbufp, dummy, sizeof(dummy));
		return sizeof(dummy);
	}
	case linux_SYSLOG_ACTION_CLEAR:
	case linux_SYSLOG_ACTION_CONSOLE_OFF:
	case linux_SYSLOG_ACTION_CONSOLE_ON:
	case linux_SYSLOG_ACTION_CONSOLE_LEVEL:
	case linux_SYSLOG_ACTION_SIZE_UNREAD:
	case linux_SYSLOG_ACTION_SIZE_BUFFER:
	default:
		break;
	}

	fixme("stub syslog");
	return 0;
}

static uid_t linux_getuid(SysFrm *)
{
	return thisProcess->Security.Real.UserID;
}

static gid_t linux_getgid(SysFrm *)
{
	return thisProcess->Security.Real.GroupID;
}

static uid_t linux_geteuid(SysFrm *)
{
	return thisProcess->Security.Effective.UserID;
}

static gid_t linux_getegid(SysFrm *)
{
	return thisProcess->Security.Effective.GroupID;
}

static pid_t linux_getppid(SysFrm *)
{
	return thisProcess->Parent->ID;
}

static pid_t linux_getpgid(SysFrm *, pid_t pid)
{
	PCB *pcb = thisProcess;
	if (pid == 0)
		return pcb->Security.ProcessGroupID;

	PCB *target = pcb->GetContext()->GetProcessByID(pid);
	if (!target)
		return -linux_ESRCH;

	return target->Security.ProcessGroupID;
}

static int linux_setpgid(SysFrm *, pid_t pid, pid_t pgid)
{
	PCB *pcb = thisProcess;
	if (pid == 0)
	{
		pcb->Security.ProcessGroupID = pgid;
		return 0;
	}

	PCB *target = pcb->GetContext()->GetProcessByID(pid);
	if (!target)
		return -linux_ESRCH;

	if (pgid == 0)
	{
		target->Security.ProcessGroupID = target->ID;
		return 0;
	}

	target->Security.ProcessGroupID = pgid;
	return 0;
}

static int linux_arch_prctl(SysFrm *, int code, unsigned long addr)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	if (vma->UserCheck(addr) < 0)
		return -linux_EFAULT;

	switch (code)
	{
	case linux_ARCH_SET_GS:
	{
#if defined(__amd64__) || defined(__i386__)
		CPU::x86::wrmsr(CPU::x86::MSRID::MSR_GS_BASE, addr);
#endif
		return 0;
	}
	case linux_ARCH_SET_FS:
	{
#if defined(__amd64__) || defined(__i386__)
		CPU::x86::wrmsr(CPU::x86::MSRID::MSR_FS_BASE, addr);
#endif
		return 0;
	}
	case linux_ARCH_GET_FS:
	{
#if defined(__amd64__) || defined(__i386__)
		*reinterpret_cast<uint64_t *>(addr) =
			CPU::x86::rdmsr(CPU::x86::MSRID::MSR_FS_BASE);
#endif
		return 0;
	}
	case linux_ARCH_GET_GS:
	{
#if defined(__amd64__) || defined(__i386__)
		*reinterpret_cast<uint64_t *>(addr) =
			CPU::x86::rdmsr(CPU::x86::MSRID::MSR_GS_BASE);
#endif
		return 0;
	}
	case linux_ARCH_GET_CPUID:
	case linux_ARCH_SET_CPUID:
	case linux_ARCH_GET_XCOMP_SUPP:
	case linux_ARCH_GET_XCOMP_PERM:
	case linux_ARCH_REQ_XCOMP_PERM:
	case linux_ARCH_GET_XCOMP_GUEST_PERM:
	case linux_ARCH_REQ_XCOMP_GUEST_PERM:
	case linux_ARCH_XCOMP_TILECFG:
	case linux_ARCH_XCOMP_TILEDATA:
	case linux_ARCH_MAP_VDSO_X32:
	case linux_ARCH_MAP_VDSO_32:
	case linux_ARCH_MAP_VDSO_64:
	case linux_ARCH_GET_UNTAG_MASK:
	case linux_ARCH_ENABLE_TAGGED_ADDR:
	case linux_ARCH_GET_MAX_TAG_BITS:
	case linux_ARCH_FORCE_TAGGED_SVA:
	{
		fixme("Code %#lx not implemented", code);
		return -linux_ENOSYS;
	}
	default:
	{
		debug("Invalid code %#lx", code);
		return -linux_EINVAL;
	}
	}
}

static int linux_reboot(SysFrm *, int magic, int magic2, int cmd, void *arg)
{
	if (magic != linux_LINUX_REBOOT_MAGIC1 ||
		(magic2 != linux_LINUX_REBOOT_MAGIC2 &&
		 magic2 != linux_LINUX_REBOOT_MAGIC2A &&
		 magic2 != linux_LINUX_REBOOT_MAGIC2B &&
		 magic2 != linux_LINUX_REBOOT_MAGIC2C))
	{
		warn("Invalid magic %#x %#x", magic, magic2);
		return -linux_EINVAL;
	}

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	debug("cmd=%#x arg=%#lx", cmd, arg);
	switch ((unsigned int)cmd)
	{
	case linux_LINUX_REBOOT_CMD_RESTART:
	{
		klog("Restarting system.");

		Tasking::Task *ctx = pcb->GetContext();
		ctx->CreateThread(ctx->GetKernelProcess(),
						  Tasking::IP(KST_Reboot))
			->Rename("Restart");
		return 0;
	}
	case linux_LINUX_REBOOT_CMD_HALT:
	{
		klog("System halted.");

		pcb->GetContext()->Panic();
		CPU::Stop();
	}
	case linux_LINUX_REBOOT_CMD_POWER_OFF:
	{
		klog("Power down.");

		Tasking::Task *ctx = pcb->GetContext();
		ctx->CreateThread(ctx->GetKernelProcess(),
						  Tasking::IP(KST_Shutdown))
			->Rename("Shutdown");
		return 0;
	}
	case linux_LINUX_REBOOT_CMD_RESTART2:
	{
		void *pArg = vma->__UserCheckAndGetAddress(arg, sizeof(void *));
		if (pArg == nullptr)
			return -linux_EFAULT;

		klog("Restarting system with command '%s'",
			 (const char *)pArg);

		Tasking::Task *ctx = pcb->GetContext();
		ctx->CreateThread(ctx->GetKernelProcess(),
						  Tasking::IP(KST_Reboot))
			->Rename("Restart");
		break;
	}
	case linux_LINUX_REBOOT_CMD_CAD_ON:
	{
		fixme("Enable reboot on Ctrl+Alt+Del");
		return 0;
	}
	case linux_LINUX_REBOOT_CMD_CAD_OFF:
	{
		fixme("Disable reboot on Ctrl+Alt+Del");
		return 0;
	}
	case linux_LINUX_REBOOT_CMD_SW_SUSPEND:
	case linux_LINUX_REBOOT_CMD_KEXEC:
	{
		fixme("cmd %#x not implemented", cmd);
		return -linux_ENOSYS;
	}
	default:
	{
		debug("Invalid cmd %#x", cmd);
		return -linux_EINVAL;
	}
	}
	return 0;
}

static int linux_sigaction(SysFrm *, int signum, const k_sigaction *act,
						   k_sigaction *oldact, size_t sigsetsize)
{
	if (signum < 1 || signum > linux_SIGRTMAX ||
		signum == linux_SIGKILL || signum == linux_SIGSTOP)
	{
		debug("Invalid signal %d", signum);
		return -linux_EINVAL;
	}

	if (sigsetsize != sizeof(sigset_t))
	{
		warn("Unsupported sigsetsize %d!", sigsetsize);
		return -linux_EINVAL;
	}

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	debug("signum=%d act=%#lx oldact=%#lx", signum, act, oldact);

	if (vma->UserCheck(act) < 0 && act != nullptr)
		return -linux_EFAULT;
	if (vma->UserCheck(oldact) < 0 && oldact != nullptr)
		return -linux_EFAULT;

	auto pAct = pcb->PageTable->Get(act);
	auto pOldact = pcb->PageTable->Get(oldact);
	int ret = 0;

	if (pOldact)
	{
		signal_t nSig = ConvertSignalToNative(signum);
		assert(nSig != SIGNULL);

		SignalAction nSA{};
		SetSigActToNative(pOldact, &nSA);
		ret = pcb->Signals.GetAction(nSig, &nSA);
		SetSigActToLinux(&nSA, pOldact);
	}

	if (unlikely(ret < 0))
		return ConvertErrnoToLinux(ret);

	if (pAct)
	{
		if (pAct->flags & linux_SA_IMMUTABLE)
		{
			warn("Immutable signal %d", signum);
			return -linux_EINVAL;
		}

		signal_t nSig = ConvertSignalToNative(signum);
		assert(nSig != SIGNULL);

		SignalAction nSA{};
		SetSigActToNative(pAct, &nSA);
		ret = pcb->Signals.SetAction(nSig, &nSA);
		SetSigActToLinux(&nSA, (k_sigaction *)pAct);
	}

	return ConvertErrnoToLinux(ret);
}

static int linux_sigprocmask(SysFrm *, int how, const sigset_t *set,
							 sigset_t *oldset, size_t sigsetsize)
{
	static_assert(sizeof(sigset_t) < PAGE_SIZE);

	if (sigsetsize != sizeof(sigset_t))
	{
		warn("Unsupported sigsetsize %d!", sigsetsize);
		return -linux_EINVAL;
	}

	TCB *tcb = thisThread;
	PCB *pcb = tcb->Parent;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	if (vma->UserCheck(set) < 0 && set != nullptr)
		return -linux_EFAULT;
	if (vma->UserCheck(oldset) < 0 && oldset != nullptr)
		return -linux_EFAULT;

	const sigset_t *pSet = (const sigset_t *)pcb->PageTable->Get((void *)set);
	sigset_t *pOldset = (sigset_t *)pcb->PageTable->Get(oldset);

	debug("how=%#x set=%#lx oldset=%#lx",
		  how, pSet ? *pSet : 0, pOldset ? *pOldset : 0);

	if (pOldset)
	{
		*pOldset = tcb->Signals.GetMask();
		*pOldset = ConvertMaskToLinux(*pOldset);
	}

	if (!pSet)
		return 0;

	sigset_t nativeSet = ConvertMaskToNative(*pSet);
	switch (how)
	{
	case SIG_BLOCK:
		tcb->Signals.Block(nativeSet);
		break;
	case SIG_UNBLOCK:
		tcb->Signals.Unblock(nativeSet);
		break;
	case SIG_SETMASK:
		tcb->Signals.SetMask(nativeSet);
		break;
	default:
		warn("Invalid how %#x", how);
		return -linux_EINVAL;
	}
	return 0;
}

static void linux_sigreturn(SysFrm *sf)
{
	thisProcess->Signals.RestoreHandleSignal(sf, thisThread);
}

static pid_t linux_gettid(SysFrm *)
{
	return thisThread->ID;
}

static int linux_tkill(SysFrm *, int tid, int sig)
{
	Tasking::TCB *tcb = thisProcess->GetThread(tid);
	if (!tcb)
		return -linux_ESRCH;

	signal_t nSig = ConvertSignalToNative(sig);
	assert(nSig != SIGNULL);
	return ConvertErrnoToLinux(tcb->SendSignal(nSig));
}

static int linux_sched_setaffinity(SysFrm *, pid_t pid, size_t cpusetsize, const cpu_set_t *mask)
{
	PCB *pcb = thisProcess;
	TCB *tcb;
	if (pid == 0)
		tcb = thisThread;
	else
		tcb = pcb->GetThread(pid);

	if (!tcb)
		return -linux_ESRCH;

	if (cpusetsize < sizeof(cpu_set_t))
		return -linux_EINVAL;

	Memory::VirtualMemoryArea *vma = pcb->vma;
	auto pMask = vma->UserCheckAndGetAddress(mask);
	if (pMask == nullptr && mask != nullptr)
		return -linux_EFAULT;

	for (size_t i = 0; i < MAX_CPU && i < CPU_SETSIZE; ++i)
	{
		if (CPU_ISSET(i, pMask))
			tcb->Info.Affinity[i] = true;
		else
			tcb->Info.Affinity[i] = false;
	}

#ifdef DEBUG
	for (size_t i = 0; i < MAX_CPU && i < CPU_SETSIZE; ++i)
	{
		bool isset = CPU_ISSET(i, pMask);
		bool affinity = tcb->Info.Affinity[i];
		if (isset != affinity)
		{
			fixme("sched_setaffinity check failed %d != %d (cpu %d)",
				  isset, affinity, i);
			assert(isset == affinity);
		}
	}
#endif
	return 0;
}

static int linux_sched_getaffinity(SysFrm *, pid_t pid, size_t cpusetsize, cpu_set_t *mask)
{
	PCB *pcb = thisProcess;
	TCB *tcb;
	if (pid == 0)
		tcb = thisThread;
	else
		tcb = pcb->GetThread(pid);

	if (!tcb)
		return -linux_ESRCH;

	if (cpusetsize < sizeof(cpu_set_t))
		return -linux_EINVAL;

	Memory::VirtualMemoryArea *vma = pcb->vma;
	auto pMask = vma->UserCheckAndGetAddress(mask);
	if (pMask == nullptr && mask != nullptr)
		return -linux_EFAULT;

	CPU_ZERO(pMask);

	for (size_t i = 0; i < MAX_CPU && i < CPU_SETSIZE; ++i)
	{
		if (tcb->Info.Affinity[i])
			CPU_SET(i, pMask);
	}

	return 0;
}

static pid_t linux_set_tid_address(SysFrm *, int *tidptr)
{
	if (tidptr == nullptr)
		return -linux_EINVAL;

	Tasking::TCB *tcb = thisThread;

	tcb->Linux.clear_child_tid = tidptr;
	return tcb->ID;
}

__no_sanitize("undefined") static ssize_t linux_getdents64(SysFrm *,
														   int fd, struct linux_dirent64 *dirp, size_t count)
{
	if (unlikely(count < sizeof(struct linux_dirent64)))
	{
		debug("Invalid count %d", count);
		return -linux_EINVAL;
	}

	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	auto it = fdt->FileMap.find(fd);
	if (it == fdt->FileMap.end())
	{
		debug("Invalid fd %d", fd);
		return -linux_EBADF;
	}

	vfs::FileDescriptorTable::Fildes &fildes = it->second;
	if (!fildes.node->IsDirectory())
	{
		debug("Not a directory");
		return -ENOTDIR;
	}

	Memory::VirtualMemoryArea *vma = pcb->vma;
	auto pDirp = vma->UserCheckAndGetAddress(dirp);
	if (pDirp == nullptr)
		return -linux_EFAULT;

/* Sanity checks */
#ifdef __LP64__
	static_assert(sizeof(kdirent) == sizeof(linux_dirent64));
	static_assert(offsetof(kdirent, d_ino) == offsetof(linux_dirent64, d_ino));
	static_assert(offsetof(kdirent, d_off) == offsetof(linux_dirent64, d_off));
	static_assert(offsetof(kdirent, d_reclen) == offsetof(linux_dirent64, d_reclen));
	static_assert(offsetof(kdirent, d_type) == offsetof(linux_dirent64, d_type));
	static_assert(offsetof(kdirent, d_name) == offsetof(linux_dirent64, d_name));
#else
#warning "Not implemented for 32-bit"
#endif

	/* The structs are the same, no need for conversion. */
	ssize_t ret = fs->ReadDirectory(fildes.node, (struct kdirent *)pDirp, count, fildes.Offset, count / sizeof(kdirent));

	if (ret > 0)
		fildes.Offset += ret;

#ifdef DEBUG
	if (ret > 0)
	{
		for (size_t bpos = 0; bpos < ret;)
		{
			linux_dirent64 *d = (struct linux_dirent64 *)((char *)pDirp + bpos);
			debug("%ld: d_ino:%d d_off:%d d_reclen:%d d_type:%d(%s) d_name:\"%s\"",
				  bpos, d->d_ino, d->d_off, d->d_reclen, d->d_type,
				  (d->d_type == DT_REG)	   ? "REG"
				  : (d->d_type == DT_DIR)  ? "DIR"
				  : (d->d_type == DT_FIFO) ? "FIFO"
				  : (d->d_type == DT_SOCK) ? "SOCK"
				  : (d->d_type == DT_LNK)  ? "LNK"
				  : (d->d_type == DT_BLK)  ? "BLK"
				  : (d->d_type == DT_CHR)  ? "CHR"
										   : "???",
				  d->d_name);

			bpos += (d->d_reclen == 0) ? 1 : d->d_reclen;
		}
	}
#endif
	return ConvertErrnoToLinux(ret);
}

static int linux_clock_gettime(SysFrm *, clockid_t clockid, struct timespec *tp)
{
	static_assert(sizeof(struct timespec) < PAGE_SIZE);

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	timespec *pTp = vma->UserCheckAndGetAddress(tp);
	if (pTp == nullptr)
		return -linux_EFAULT;

	/* FIXME: This is not correct? */
	switch (clockid)
	{
	case linux_CLOCK_REALTIME:
	{
		uint64_t time = GlobalClock->Now();
		pTp->tv_sec = ToSeconds(time);
		pTp->tv_nsec = time;
		debug("time=%ld sec=%ld nsec=%ld",
			  time, pTp->tv_sec, pTp->tv_nsec);
		break;
	}
	case linux_CLOCK_MONOTONIC:
	{
		uint64_t time = GlobalClock->Now();
		pTp->tv_sec = ToSeconds(time);
		pTp->tv_nsec = time;
		debug("time=%ld sec=%ld nsec=%ld",
			  time, pTp->tv_sec, pTp->tv_nsec);
		break;
	}
	case linux_CLOCK_PROCESS_CPUTIME_ID:
	case linux_CLOCK_THREAD_CPUTIME_ID:
	case linux_CLOCK_MONOTONIC_RAW:
	case linux_CLOCK_REALTIME_COARSE:
	case linux_CLOCK_MONOTONIC_COARSE:
	case linux_CLOCK_BOOTTIME:
	case linux_CLOCK_REALTIME_ALARM:
	case linux_CLOCK_BOOTTIME_ALARM:
	case linux_CLOCK_SGI_CYCLE:
	case linux_CLOCK_TAI:
	{
		fixme("clockid %d is stub", clockid);
		return -linux_ENOSYS;
	}
	default:
	{
		warn("Invalid clockid %#lx", clockid);
		return -linux_EINVAL;
	}
	}
	return 0;
}

static int linux_clock_nanosleep(SysFrm *, clockid_t clockid, int flags,
								 const struct timespec *request,
								 struct timespec *remain)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const timespec *pRequest = vma->UserCheckAndGetAddress(request);
	timespec *pRemain = vma->UserCheckAndGetAddress(remain);
	if (pRequest == nullptr)
		return -linux_EFAULT;

	UNUSED(pRemain);
	UNUSED(flags);

	switch (clockid)
	{
	case linux_CLOCK_REALTIME:
	case linux_CLOCK_MONOTONIC:
	{
		uint64_t time = GlobalClock->Now();
		uint64_t rqTime = FromSeconds(pRequest->tv_sec) + pRequest->tv_nsec;

		debug("Sleeping for %ld", rqTime - time);
		if (rqTime > time)
			pcb->GetContext()->Sleep(rqTime - time);
		break;
	}
	case linux_CLOCK_PROCESS_CPUTIME_ID:
	case linux_CLOCK_THREAD_CPUTIME_ID:
	case linux_CLOCK_MONOTONIC_RAW:
	case linux_CLOCK_REALTIME_COARSE:
	case linux_CLOCK_MONOTONIC_COARSE:
	case linux_CLOCK_BOOTTIME:
	case linux_CLOCK_REALTIME_ALARM:
	case linux_CLOCK_BOOTTIME_ALARM:
	case linux_CLOCK_SGI_CYCLE:
	case linux_CLOCK_TAI:
	{
		fixme("clockid %d is stub", clockid);
		return -linux_ENOSYS;
	}
	default:
	{
		warn("Invalid clockid %#lx", clockid);
		return -linux_EINVAL;
	}
	}
	return 0;
}

static __noreturn void linux_exit_group(SysFrm *sf, int status)
{
	fixme("status=%d", status);
	linux_exit(sf, status);
}

static int linux_tgkill(SysFrm *sf, pid_t tgid, pid_t tid, int sig)
{
	Tasking::TCB *tcb = thisProcess->GetThread(tid);
	if (!tcb || tcb->Linux.tgid != tgid)
	{
		debug("Invalid tgid %d tid %d", tgid, tid);

		tcb = nullptr;
		for (auto t : thisProcess->Threads)
		{
			if (t->Linux.tgid == tgid)
			{
				debug("Found tgid %d tid %d", tgid, t->ID);
				tcb = t;
				break;
			}
		}

		if (!tcb)
			return -linux_ESRCH;
	}

	signal_t nSig = ConvertSignalToNative(sig);
	assert(nSig != SIGNULL);
	return ConvertErrnoToLinux(tcb->SendSignal(nSig));
}

static int linux_openat(SysFrm *, int dirfd, const char *pathname, int flags, mode_t mode)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	const char *pPathname = vma->UserCheckAndGetAddress(pathname);
	if (pPathname == nullptr)
		return -linux_EFAULT;

	debug("dirfd=%d pathname=%s flags=%#x mode=%#x",
		  dirfd, pPathname, flags, mode);

	if (dirfd == linux_AT_FDCWD)
	{
		Node absoluteNode = fs->Lookup(pcb->CWD, pPathname);
		if (!absoluteNode)
			return -linux_ENOENT;

		const char *absPath = new char[strlen(absoluteNode->Path.c_str()) + 1];
		strcpy((char *)absPath, absoluteNode->Path.c_str());
		int ret = fdt->usr_open(absPath, flags, mode);
		delete[] absPath;
		return ConvertErrnoToLinux(ret);
	}

	if (!fs->PathIsRelative(pPathname))
		return fdt->usr_open(pPathname, flags, mode);

	fixme("dirfd=%d is stub", dirfd);
	return -linux_ENOSYS;
}

static long linux_newfstatat(SysFrm *, int dirfd, const char *pathname,
							 struct linux_kstat *statbuf, int flag)
{
	PCB *pcb = thisProcess;
	vfs::FileDescriptorTable *fdt = pcb->FileDescriptors;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	if (flag)
		fixme("flag %#x is stub", flag);

	const char *pPathname = vma->UserCheckAndGetAddress(pathname);
	struct linux_kstat *pStatbuf = vma->UserCheckAndGetAddress(statbuf);
	if (pPathname == nullptr || pStatbuf == nullptr)
		return -linux_EFAULT;

	debug("\"%s\" %#lx %#lx", pPathname, pathname, statbuf);

	if (fs->PathIsAbsolute(pPathname))
	{
		debug("path \"%s\" is absolute", pPathname);
		struct kstat nstat = KStatToStat(*pStatbuf);
		int ret = fdt->usr_stat(pPathname, &nstat);
		*pStatbuf = StatToKStat(nstat);
		return ConvertErrnoToLinux(ret);
	}

	switch (dirfd)
	{
	case linux_AT_FDCWD:
	{
		debug("dirfd is AT_FDCWD for \"%s\"", pPathname);
		Node node = fs->Lookup(pcb->CWD, pPathname);
		if (!node)
			return -linux_ENOENT;

		struct kstat nstat = {};
		int ret = fdt->usr_stat(node->GetPath().c_str(), &nstat);
		*pStatbuf = StatToKStat(nstat);
		return ConvertErrnoToLinux(ret);
	}
	default:
	{
		debug("dirfd is %d for \"%s\"", dirfd, pPathname);
		auto it = fdt->FileMap.find(dirfd);
		if (it == fdt->FileMap.end())
			ReturnLogError(-linux_EBADF, "Invalid fd %d", dirfd);

		vfs::FileDescriptorTable::Fildes &fildes = it->second;
		Node node = fs->Lookup(fildes.node, pPathname);
		debug("node: %s", node->GetPath().c_str());
		if (!node)
			return -linux_ENOENT;

		struct kstat nstat = {};
		int ret = fdt->usr_stat(node->GetPath().c_str(), &nstat);
		*pStatbuf = StatToKStat(nstat);
		return ConvertErrnoToLinux(ret);
	}
	}
}

static int linux_pipe2(SysFrm *sf, int pipefd[2], int flags)
{
	if (flags == 0)
		return linux_pipe(sf, pipefd);

	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	int *pPipefd = vma->UserCheckAndGetAddress(pipefd);
	if (pPipefd == nullptr)
		return -linux_EFAULT;

	debug("pipefd=%#lx", pPipefd);
	fixme("pipefd=[%d, %d] flags=%#x", pPipefd[0], pPipefd[1], flags);
	return -linux_ENOSYS;
}

static int linux_prlimit64(SysFrm *, pid_t pid, int resource,
						   const struct rlimit *new_limit,
						   struct rlimit *old_limit)
{
	static_assert(sizeof(struct rlimit) < PAGE_SIZE);

	PCB *pcb = nullptr;
	if (likely(pid == 0))
		pcb = thisProcess;
	else
	{
		pcb = thisProcess->GetContext()->GetProcessByID(pid);
		if (pcb == nullptr)
			return -linux_ESRCH;

		fixme("implement CAP_SYS_RESOURCE");
		return -linux_EPERM;
	}
	Memory::VirtualMemoryArea *vma = thisProcess->vma;

	auto pOldLimit = vma->UserCheckAndGetAddress(old_limit);
	auto pNewLimit = vma->UserCheckAndGetAddress(new_limit);
	if (pOldLimit == nullptr && old_limit != nullptr)
		return -linux_EFAULT;

	if (pNewLimit == nullptr && new_limit != nullptr)
		return -linux_EFAULT;

	if (new_limit)
	{
		if (pNewLimit->rlim_cur > pNewLimit->rlim_max)
			return -linux_EINVAL;
	}

	switch (resource)
	{
	case linux_RLIMIT_CPU:
	case linux_RLIMIT_FSIZE:
	case linux_RLIMIT_DATA:
	case linux_RLIMIT_STACK:
	case linux_RLIMIT_CORE:
	case linux_RLIMIT_RSS:
		goto __stub;
	case linux_RLIMIT_NPROC:
	{
		if (old_limit)
		{
			pOldLimit->rlim_cur = pcb->SoftLimits.Threads;
			pOldLimit->rlim_max = pcb->HardLimits.Threads;
			debug("read NPROC limit: {%#lx, %#lx}", pOldLimit->rlim_cur, pOldLimit->rlim_max);
		}

		if (new_limit)
		{
			if (pNewLimit->rlim_max > pcb->HardLimits.Threads && !pcb->Security.CanAdjustHardLimits)
				return -linux_EPERM;

			debug("setting NPROC limit to {%#lx, %#lx}->{%#lx, %#lx}",
				  pcb->SoftLimits.Threads, pcb->HardLimits.Threads,
				  pNewLimit->rlim_cur, pNewLimit->rlim_max);
			pcb->SoftLimits.Threads = pNewLimit->rlim_cur;
			pcb->HardLimits.Threads = pNewLimit->rlim_max;
		}
		return 0;
	}
	case linux_RLIMIT_NOFILE:
	{
		if (old_limit)
		{
			pOldLimit->rlim_cur = pcb->SoftLimits.OpenFiles;
			pOldLimit->rlim_max = pcb->HardLimits.OpenFiles;
			debug("read NOFILE limit: {%#lx, %#lx}", pOldLimit->rlim_cur, pOldLimit->rlim_max);
		}

		if (new_limit)
		{
			if (pNewLimit->rlim_max > pcb->HardLimits.OpenFiles && !pcb->Security.CanAdjustHardLimits)
				return -linux_EPERM;

			debug("setting NOFILE limit to {%#lx, %#lx}->{%#lx, %#lx}",
				  pcb->SoftLimits.OpenFiles, pcb->HardLimits.OpenFiles,
				  pNewLimit->rlim_cur, pNewLimit->rlim_max);
			pcb->SoftLimits.OpenFiles = pNewLimit->rlim_cur;
			pcb->HardLimits.OpenFiles = pNewLimit->rlim_max;
		}
		return 0;
	}
	case linux_RLIMIT_MEMLOCK:
		goto __stub;
	case linux_RLIMIT_AS:
	{
		if (old_limit)
		{
			pOldLimit->rlim_cur = pcb->SoftLimits.Memory;
			pOldLimit->rlim_max = pcb->HardLimits.Memory;
			debug("read AS limit: {%#lx, %#lx}", pOldLimit->rlim_cur, pOldLimit->rlim_max);
		}

		if (new_limit)
		{
			if (pNewLimit->rlim_max > pcb->HardLimits.Memory && !pcb->Security.CanAdjustHardLimits)
				return -linux_EPERM;

			debug("setting AS limit to {%#lx, %#lx}->{%#lx, %#lx}",
				  pcb->SoftLimits.Memory, pcb->HardLimits.Memory,
				  pNewLimit->rlim_cur, pNewLimit->rlim_max);
			pcb->SoftLimits.Memory = pNewLimit->rlim_cur;
			pcb->HardLimits.Memory = pNewLimit->rlim_max;
		}
		return 0;
	}
	case linux_RLIMIT_LOCKS:
	case linux_RLIMIT_SIGPENDING:
	case linux_RLIMIT_MSGQUEUE:
	case linux_RLIMIT_NICE:
	case linux_RLIMIT_RTPRIO:
	case linux_RLIMIT_RTTIME:
	case linux_RLIMIT_NLIMITS:
	__stub:
		{
			fixme("resource %s(%d) is stub", rlimitStr[resource], resource);
			return 0; /* just return 0 */
		}
	default:
	{
		debug("Invalid resource %d", resource);
		return -linux_EINVAL;
	}
	}
}

static ssize_t linux_getrandom(SysFrm *, void *buf,
							   size_t buflen, unsigned int flags)
{
	PCB *pcb = thisProcess;
	Memory::VirtualMemoryArea *vma = pcb->vma;

	if (flags & linux_GRND_NONBLOCK)
		fixme("GRND_NONBLOCK not implemented");

	if (flags & ~(linux_GRND_NONBLOCK |
				  linux_GRND_RANDOM |
				  linux_GRND_INSECURE))
	{
		warn("Invalid flags %#x", flags);
		return -linux_EINVAL;
	}

	auto pBuf = vma->UserCheckAndGetAddress(buf, buflen);
	if (pBuf == nullptr)
		return -linux_EFAULT;

	if (flags & linux_GRND_RANDOM)
	{
		uint16_t random;
		for (size_t i = 0; i < buflen; i++)
		{
			random = Random::rand16();
			((uint8_t *)pBuf)[i] = uint8_t(random & 0xFF);
		}
		return buflen;
	}

	return 0;
}

static SyscallData LinuxSyscallsTableAMD64[] = {
	[__NR_amd64_read] = {"read", (void *)linux_read},
	[__NR_amd64_write] = {"write", (void *)linux_write},
	[__NR_amd64_open] = {"open", (void *)linux_open},
	[__NR_amd64_close] = {"close", (void *)linux_close},
	[__NR_amd64_stat] = {"stat", (void *)linux_stat},
	[__NR_amd64_fstat] = {"fstat", (void *)linux_fstat},
	[__NR_amd64_lstat] = {"lstat", (void *)linux_lstat},
	[__NR_amd64_poll] = {"poll", (void *)linux_poll},
	[__NR_amd64_lseek] = {"lseek", (void *)linux_lseek},
	[__NR_amd64_mmap] = {"mmap", (void *)linux_mmap},
	[__NR_amd64_mprotect] = {"mprotect", (void *)linux_mprotect},
	[__NR_amd64_munmap] = {"munmap", (void *)linux_munmap},
	[__NR_amd64_brk] = {"brk", (void *)linux_brk},
	[__NR_amd64_rt_sigaction] = {"rt_sigaction", (void *)linux_sigaction},
	[__NR_amd64_rt_sigprocmask] = {"rt_sigprocmask", (void *)linux_sigprocmask},
	[__NR_amd64_rt_sigreturn] = {"rt_sigreturn", (void *)linux_sigreturn},
	[__NR_amd64_ioctl] = {"ioctl", (void *)linux_ioctl},
	[__NR_amd64_pread64] = {"pread64", (void *)linux_pread64},
	[__NR_amd64_pwrite64] = {"pwrite64", (void *)linux_pwrite64},
	[__NR_amd64_readv] = {"readv", (void *)linux_readv},
	[__NR_amd64_writev] = {"writev", (void *)linux_writev},
	[__NR_amd64_access] = {"access", (void *)linux_access},
	[__NR_amd64_pipe] = {"pipe", (void *)linux_pipe},
	[__NR_amd64_select] = {"select", (void *)nullptr},
	[__NR_amd64_sched_yield] = {"sched_yield", (void *)nullptr},
	[__NR_amd64_mremap] = {"mremap", (void *)nullptr},
	[__NR_amd64_msync] = {"msync", (void *)nullptr},
	[__NR_amd64_mincore] = {"mincore", (void *)nullptr},
	[__NR_amd64_madvise] = {"madvise", (void *)linux_madvise},
	[__NR_amd64_shmget] = {"shmget", (void *)nullptr},
	[__NR_amd64_shmat] = {"shmat", (void *)nullptr},
	[__NR_amd64_shmctl] = {"shmctl", (void *)nullptr},
	[__NR_amd64_dup] = {"dup", (void *)linux_dup},
	[__NR_amd64_dup2] = {"dup2", (void *)linux_dup2},
	[__NR_amd64_pause] = {"pause", (void *)linux_pause},
	[__NR_amd64_nanosleep] = {"nanosleep", (void *)linux_nanosleep},
	[__NR_amd64_getitimer] = {"getitimer", (void *)nullptr},
	[__NR_amd64_alarm] = {"alarm", (void *)nullptr},
	[__NR_amd64_setitimer] = {"setitimer", (void *)linux_setitimer},
	[__NR_amd64_getpid] = {"getpid", (void *)linux_getpid},
	[__NR_amd64_sendfile] = {"sendfile", (void *)nullptr},
	[__NR_amd64_socket] = {"socket", (void *)nullptr},
	[__NR_amd64_connect] = {"connect", (void *)nullptr},
	[__NR_amd64_accept] = {"accept", (void *)nullptr},
	[__NR_amd64_sendto] = {"sendto", (void *)nullptr},
	[__NR_amd64_recvfrom] = {"recvfrom", (void *)nullptr},
	[__NR_amd64_sendmsg] = {"sendmsg", (void *)nullptr},
	[__NR_amd64_recvmsg] = {"recvmsg", (void *)nullptr},
	[__NR_amd64_shutdown] = {"shutdown", (void *)linux_shutdown},
	[__NR_amd64_bind] = {"bind", (void *)nullptr},
	[__NR_amd64_listen] = {"listen", (void *)nullptr},
	[__NR_amd64_getsockname] = {"getsockname", (void *)nullptr},
	[__NR_amd64_getpeername] = {"getpeername", (void *)nullptr},
	[__NR_amd64_socketpair] = {"socketpair", (void *)nullptr},
	[__NR_amd64_setsockopt] = {"setsockopt", (void *)nullptr},
	[__NR_amd64_getsockopt] = {"getsockopt", (void *)nullptr},
	[__NR_amd64_clone] = {"clone", (void *)nullptr},
	[__NR_amd64_fork] = {"fork", (void *)linux_fork},
	[__NR_amd64_vfork] = {"vfork", (void *)linux_vfork},
	[__NR_amd64_execve] = {"execve", (void *)linux_execve},
	[__NR_amd64_exit] = {"exit", (void *)linux_exit},
	[__NR_amd64_wait4] = {"wait4", (void *)linux_wait4},
	[__NR_amd64_kill] = {"kill", (void *)linux_kill},
	[__NR_amd64_uname] = {"uname", (void *)linux_uname},
	[__NR_amd64_semget] = {"semget", (void *)nullptr},
	[__NR_amd64_semop] = {"semop", (void *)nullptr},
	[__NR_amd64_semctl] = {"semctl", (void *)nullptr},
	[__NR_amd64_shmdt] = {"shmdt", (void *)nullptr},
	[__NR_amd64_msgget] = {"msgget", (void *)nullptr},
	[__NR_amd64_msgsnd] = {"msgsnd", (void *)nullptr},
	[__NR_amd64_msgrcv] = {"msgrcv", (void *)nullptr},
	[__NR_amd64_msgctl] = {"msgctl", (void *)nullptr},
	[__NR_amd64_fcntl] = {"fcntl", (void *)linux_fcntl},
	[__NR_amd64_flock] = {"flock", (void *)nullptr},
	[__NR_amd64_fsync] = {"fsync", (void *)nullptr},
	[__NR_amd64_fdatasync] = {"fdatasync", (void *)nullptr},
	[__NR_amd64_truncate] = {"truncate", (void *)nullptr},
	[__NR_amd64_ftruncate] = {"ftruncate", (void *)nullptr},
	[__NR_amd64_getdents] = {"getdents", (void *)nullptr},
	[__NR_amd64_getcwd] = {"getcwd", (void *)linux_getcwd},
	[__NR_amd64_chdir] = {"chdir", (void *)linux_chdir},
	[__NR_amd64_fchdir] = {"fchdir", (void *)linux_fchdir},
	[__NR_amd64_rename] = {"rename", (void *)nullptr},
	[__NR_amd64_mkdir] = {"mkdir", (void *)linux_mkdir},
	[__NR_amd64_rmdir] = {"rmdir", (void *)nullptr},
	[__NR_amd64_creat] = {"creat", (void *)linux_creat},
	[__NR_amd64_link] = {"link", (void *)nullptr},
	[__NR_amd64_unlink] = {"unlink", (void *)nullptr},
	[__NR_amd64_symlink] = {"symlink", (void *)nullptr},
	[__NR_amd64_readlink] = {"readlink", (void *)linux_readlink},
	[__NR_amd64_chmod] = {"chmod", (void *)linux_chmod},
	[__NR_amd64_fchmod] = {"fchmod", (void *)linux_fchmod},
	[__NR_amd64_chown] = {"chown", (void *)nullptr},
	[__NR_amd64_fchown] = {"fchown", (void *)nullptr},
	[__NR_amd64_lchown] = {"lchown", (void *)nullptr},
	[__NR_amd64_umask] = {"umask", (void *)linux_umask},
	[__NR_amd64_gettimeofday] = {"gettimeofday", (void *)nullptr},
	[__NR_amd64_getrlimit] = {"getrlimit", (void *)nullptr},
	[__NR_amd64_getrusage] = {"getrusage", (void *)linux_getrusage},
	[__NR_amd64_sysinfo] = {"sysinfo", (void *)linux_sysinfo},
	[__NR_amd64_times] = {"times", (void *)nullptr},
	[__NR_amd64_ptrace] = {"ptrace", (void *)nullptr},
	[__NR_amd64_getuid] = {"getuid", (void *)linux_getuid},
	[__NR_amd64_syslog] = {"syslog", (void *)linux_syslog},
	[__NR_amd64_getgid] = {"getgid", (void *)linux_getgid},
	[__NR_amd64_setuid] = {"setuid", (void *)nullptr},
	[__NR_amd64_setgid] = {"setgid", (void *)nullptr},
	[__NR_amd64_geteuid] = {"geteuid", (void *)linux_geteuid},
	[__NR_amd64_getegid] = {"getegid", (void *)linux_getegid},
	[__NR_amd64_setpgid] = {"setpgid", (void *)linux_setpgid},
	[__NR_amd64_getppid] = {"getppid", (void *)linux_getppid},
	[__NR_amd64_getpgrp] = {"getpgrp", (void *)nullptr},
	[__NR_amd64_setsid] = {"setsid", (void *)nullptr},
	[__NR_amd64_setreuid] = {"setreuid", (void *)nullptr},
	[__NR_amd64_setregid] = {"setregid", (void *)nullptr},
	[__NR_amd64_getgroups] = {"getgroups", (void *)nullptr},
	[__NR_amd64_setgroups] = {"setgroups", (void *)nullptr},
	[__NR_amd64_setresuid] = {"setresuid", (void *)nullptr},
	[__NR_amd64_getresuid] = {"getresuid", (void *)nullptr},
	[__NR_amd64_setresgid] = {"setresgid", (void *)nullptr},
	[__NR_amd64_getresgid] = {"getresgid", (void *)nullptr},
	[__NR_amd64_getpgid] = {"getpgid", (void *)linux_getpgid},
	[__NR_amd64_setfsuid] = {"setfsuid", (void *)nullptr},
	[__NR_amd64_setfsgid] = {"setfsgid", (void *)nullptr},
	[__NR_amd64_getsid] = {"getsid", (void *)nullptr},
	[__NR_amd64_capget] = {"capget", (void *)nullptr},
	[__NR_amd64_capset] = {"capset", (void *)nullptr},
	[__NR_amd64_rt_sigpending] = {"rt_sigpending", (void *)nullptr},
	[__NR_amd64_rt_sigtimedwait] = {"rt_sigtimedwait", (void *)nullptr},
	[__NR_amd64_rt_sigqueueinfo] = {"rt_sigqueueinfo", (void *)nullptr},
	[__NR_amd64_rt_sigsuspend] = {"rt_sigsuspend", (void *)nullptr},
	[__NR_amd64_sigaltstack] = {"sigaltstack", (void *)nullptr},
	[__NR_amd64_utime] = {"utime", (void *)nullptr},
	[__NR_amd64_mknod] = {"mknod", (void *)nullptr},
	[__NR_amd64_uselib] = {"uselib", (void *)nullptr},
	[__NR_amd64_personality] = {"personality", (void *)nullptr},
	[__NR_amd64_ustat] = {"ustat", (void *)nullptr},
	[__NR_amd64_statfs] = {"statfs", (void *)nullptr},
	[__NR_amd64_fstatfs] = {"fstatfs", (void *)nullptr},
	[__NR_amd64_sysfs] = {"sysfs", (void *)nullptr},
	[__NR_amd64_getpriority] = {"getpriority", (void *)nullptr},
	[__NR_amd64_setpriority] = {"setpriority", (void *)nullptr},
	[__NR_amd64_sched_setparam] = {"sched_setparam", (void *)nullptr},
	[__NR_amd64_sched_getparam] = {"sched_getparam", (void *)nullptr},
	[__NR_amd64_sched_setscheduler] = {"sched_setscheduler", (void *)nullptr},
	[__NR_amd64_sched_getscheduler] = {"sched_getscheduler", (void *)nullptr},
	[__NR_amd64_sched_get_priority_max] = {"sched_get_priority_max", (void *)nullptr},
	[__NR_amd64_sched_get_priority_min] = {"sched_get_priority_min", (void *)nullptr},
	[__NR_amd64_sched_rr_get_interval] = {"sched_rr_get_interval", (void *)nullptr},
	[__NR_amd64_mlock] = {"mlock", (void *)nullptr},
	[__NR_amd64_munlock] = {"munlock", (void *)nullptr},
	[__NR_amd64_mlockall] = {"mlockall", (void *)nullptr},
	[__NR_amd64_munlockall] = {"munlockall", (void *)nullptr},
	[__NR_amd64_vhangup] = {"vhangup", (void *)nullptr},
	[__NR_amd64_modify_ldt] = {"modify_ldt", (void *)nullptr},
	[__NR_amd64_pivot_root] = {"pivot_root", (void *)nullptr},
	[__NR_amd64__sysctl] = {"_sysctl", (void *)nullptr},
	[__NR_amd64_prctl] = {"prctl", (void *)nullptr},
	[__NR_amd64_arch_prctl] = {"arch_prctl", (void *)linux_arch_prctl},
	[__NR_amd64_adjtimex] = {"adjtimex", (void *)nullptr},
	[__NR_amd64_setrlimit] = {"setrlimit", (void *)nullptr},
	[__NR_amd64_chroot] = {"chroot", (void *)nullptr},
	[__NR_amd64_sync] = {"sync", (void *)nullptr},
	[__NR_amd64_acct] = {"acct", (void *)nullptr},
	[__NR_amd64_settimeofday] = {"settimeofday", (void *)nullptr},
	[__NR_amd64_mount] = {"mount", (void *)nullptr},
	[__NR_amd64_umount2] = {"umount2", (void *)nullptr},
	[__NR_amd64_swapon] = {"swapon", (void *)nullptr},
	[__NR_amd64_swapoff] = {"swapoff", (void *)nullptr},
	[__NR_amd64_reboot] = {"reboot", (void *)linux_reboot},
	[__NR_amd64_sethostname] = {"sethostname", (void *)nullptr},
	[__NR_amd64_setdomainname] = {"setdomainname", (void *)nullptr},
	[__NR_amd64_iopl] = {"iopl", (void *)nullptr},
	[__NR_amd64_ioperm] = {"ioperm", (void *)nullptr},
	[__NR_amd64_create_module] = {"create_module", (void *)nullptr},
	[__NR_amd64_init_module] = {"init_module", (void *)nullptr},
	[__NR_amd64_delete_module] = {"delete_module", (void *)nullptr},
	[__NR_amd64_get_kernel_syms] = {"get_kernel_syms", (void *)nullptr},
	[__NR_amd64_query_module] = {"query_module", (void *)nullptr},
	[__NR_amd64_quotactl] = {"quotactl", (void *)nullptr},
	[__NR_amd64_nfsservctl] = {"nfsservctl", (void *)nullptr},
	[__NR_amd64_getpmsg] = {"getpmsg", (void *)nullptr},
	[__NR_amd64_putpmsg] = {"putpmsg", (void *)nullptr},
	[__NR_amd64_afs_syscall] = {"afs_syscall", (void *)nullptr},
	[__NR_amd64_tuxcall] = {"tuxcall", (void *)nullptr},
	[__NR_amd64_security] = {"security", (void *)nullptr},
	[__NR_amd64_gettid] = {"gettid", (void *)linux_gettid},
	[__NR_amd64_readahead] = {"readahead", (void *)nullptr},
	[__NR_amd64_setxattr] = {"setxattr", (void *)nullptr},
	[__NR_amd64_lsetxattr] = {"lsetxattr", (void *)nullptr},
	[__NR_amd64_fsetxattr] = {"fsetxattr", (void *)nullptr},
	[__NR_amd64_getxattr] = {"getxattr", (void *)nullptr},
	[__NR_amd64_lgetxattr] = {"lgetxattr", (void *)nullptr},
	[__NR_amd64_fgetxattr] = {"fgetxattr", (void *)nullptr},
	[__NR_amd64_listxattr] = {"listxattr", (void *)nullptr},
	[__NR_amd64_llistxattr] = {"llistxattr", (void *)nullptr},
	[__NR_amd64_flistxattr] = {"flistxattr", (void *)nullptr},
	[__NR_amd64_removexattr] = {"removexattr", (void *)nullptr},
	[__NR_amd64_lremovexattr] = {"lremovexattr", (void *)nullptr},
	[__NR_amd64_fremovexattr] = {"fremovexattr", (void *)nullptr},
	[__NR_amd64_tkill] = {"tkill", (void *)linux_tkill},
	[__NR_amd64_time] = {"time", (void *)nullptr},
	[__NR_amd64_futex] = {"futex", (void *)nullptr},
	[__NR_amd64_sched_setaffinity] = {"sched_setaffinity", (void *)linux_sched_setaffinity},
	[__NR_amd64_sched_getaffinity] = {"sched_getaffinity", (void *)linux_sched_getaffinity},
	[__NR_amd64_set_thread_area] = {"set_thread_area", (void *)nullptr},
	[__NR_amd64_io_setup] = {"io_setup", (void *)nullptr},
	[__NR_amd64_io_destroy] = {"io_destroy", (void *)nullptr},
	[__NR_amd64_io_getevents] = {"io_getevents", (void *)nullptr},
	[__NR_amd64_io_submit] = {"io_submit", (void *)nullptr},
	[__NR_amd64_io_cancel] = {"io_cancel", (void *)nullptr},
	[__NR_amd64_get_thread_area] = {"get_thread_area", (void *)nullptr},
	[__NR_amd64_lookup_dcookie] = {"lookup_dcookie", (void *)nullptr},
	[__NR_amd64_epoll_create] = {"epoll_create", (void *)nullptr},
	[__NR_amd64_epoll_ctl_old] = {"epoll_ctl_old", (void *)nullptr},
	[__NR_amd64_epoll_wait_old] = {"epoll_wait_old", (void *)nullptr},
	[__NR_amd64_remap_file_pages] = {"remap_file_pages", (void *)nullptr},
	[__NR_amd64_getdents64] = {"getdents64", (void *)linux_getdents64},
	[__NR_amd64_set_tid_address] = {"set_tid_address", (void *)linux_set_tid_address},
	[__NR_amd64_restart_syscall] = {"restart_syscall", (void *)nullptr},
	[__NR_amd64_semtimedop] = {"semtimedop", (void *)nullptr},
	[__NR_amd64_fadvise64] = {"fadvise64", (void *)nullptr},
	[__NR_amd64_timer_create] = {"timer_create", (void *)nullptr},
	[__NR_amd64_timer_settime] = {"timer_settime", (void *)nullptr},
	[__NR_amd64_timer_gettime] = {"timer_gettime", (void *)nullptr},
	[__NR_amd64_timer_getoverrun] = {"timer_getoverrun", (void *)nullptr},
	[__NR_amd64_timer_delete] = {"timer_delete", (void *)nullptr},
	[__NR_amd64_clock_settime] = {"clock_settime", (void *)nullptr},
	[__NR_amd64_clock_gettime] = {"clock_gettime", (void *)linux_clock_gettime},
	[__NR_amd64_clock_getres] = {"clock_getres", (void *)nullptr},
	[__NR_amd64_clock_nanosleep] = {"clock_nanosleep", (void *)linux_clock_nanosleep},
	[__NR_amd64_exit_group] = {"exit_group", (void *)linux_exit_group},
	[__NR_amd64_epoll_wait] = {"epoll_wait", (void *)nullptr},
	[__NR_amd64_epoll_ctl] = {"epoll_ctl", (void *)nullptr},
	[__NR_amd64_tgkill] = {"tgkill", (void *)linux_tgkill},
	[__NR_amd64_utimes] = {"utimes", (void *)nullptr},
	[__NR_amd64_vserver] = {"vserver", (void *)nullptr},
	[__NR_amd64_mbind] = {"mbind", (void *)nullptr},
	[__NR_amd64_set_mempolicy] = {"set_mempolicy", (void *)nullptr},
	[__NR_amd64_get_mempolicy] = {"get_mempolicy", (void *)nullptr},
	[__NR_amd64_mq_open] = {"mq_open", (void *)nullptr},
	[__NR_amd64_mq_unlink] = {"mq_unlink", (void *)nullptr},
	[__NR_amd64_mq_timedsend] = {"mq_timedsend", (void *)nullptr},
	[__NR_amd64_mq_timedreceive] = {"mq_timedreceive", (void *)nullptr},
	[__NR_amd64_mq_notify] = {"mq_notify", (void *)nullptr},
	[__NR_amd64_mq_getsetattr] = {"mq_getsetattr", (void *)nullptr},
	[__NR_amd64_kexec_load] = {"kexec_load", (void *)nullptr},
	[__NR_amd64_waitid] = {"waitid", (void *)nullptr},
	[__NR_amd64_add_key] = {"add_key", (void *)nullptr},
	[__NR_amd64_request_key] = {"request_key", (void *)nullptr},
	[__NR_amd64_keyctl] = {"keyctl", (void *)nullptr},
	[__NR_amd64_ioprio_set] = {"ioprio_set", (void *)nullptr},
	[__NR_amd64_ioprio_get] = {"ioprio_get", (void *)nullptr},
	[__NR_amd64_inotify_init] = {"inotify_init", (void *)nullptr},
	[__NR_amd64_inotify_add_watch] = {"inotify_add_watch", (void *)nullptr},
	[__NR_amd64_inotify_rm_watch] = {"inotify_rm_watch", (void *)nullptr},
	[__NR_amd64_migrate_pages] = {"migrate_pages", (void *)nullptr},
	[__NR_amd64_openat] = {"openat", (void *)linux_openat},
	[__NR_amd64_mkdirat] = {"mkdirat", (void *)nullptr},
	[__NR_amd64_mknodat] = {"mknodat", (void *)nullptr},
	[__NR_amd64_fchownat] = {"fchownat", (void *)nullptr},
	[__NR_amd64_futimesat] = {"futimesat", (void *)nullptr},
	[__NR_amd64_newfstatat] = {"newfstatat", (void *)linux_newfstatat},
	[__NR_amd64_unlinkat] = {"unlinkat", (void *)nullptr},
	[__NR_amd64_renameat] = {"renameat", (void *)nullptr},
	[__NR_amd64_linkat] = {"linkat", (void *)nullptr},
	[__NR_amd64_symlinkat] = {"symlinkat", (void *)nullptr},
	[__NR_amd64_readlinkat] = {"readlinkat", (void *)nullptr},
	[__NR_amd64_fchmodat] = {"fchmodat", (void *)nullptr},
	[__NR_amd64_faccessat] = {"faccessat", (void *)nullptr},
	[__NR_amd64_pselect6] = {"pselect6", (void *)nullptr},
	[__NR_amd64_ppoll] = {"ppoll", (void *)nullptr},
	[__NR_amd64_unshare] = {"unshare", (void *)nullptr},
	[__NR_amd64_set_robust_list] = {"set_robust_list", (void *)nullptr},
	[__NR_amd64_get_robust_list] = {"get_robust_list", (void *)nullptr},
	[__NR_amd64_splice] = {"splice", (void *)nullptr},
	[__NR_amd64_tee] = {"tee", (void *)nullptr},
	[__NR_amd64_sync_file_range] = {"sync_file_range", (void *)nullptr},
	[__NR_amd64_vmsplice] = {"vmsplice", (void *)nullptr},
	[__NR_amd64_move_pages] = {"move_pages", (void *)nullptr},
	[__NR_amd64_utimensat] = {"utimensat", (void *)nullptr},
	[__NR_amd64_epoll_pwait] = {"epoll_pwait", (void *)nullptr},
	[__NR_amd64_signalfd] = {"signalfd", (void *)nullptr},
	[__NR_amd64_timerfd_create] = {"timerfd_create", (void *)nullptr},
	[__NR_amd64_eventfd] = {"eventfd", (void *)nullptr},
	[__NR_amd64_fallocate] = {"fallocate", (void *)nullptr},
	[__NR_amd64_timerfd_settime] = {"timerfd_settime", (void *)nullptr},
	[__NR_amd64_timerfd_gettime] = {"timerfd_gettime", (void *)nullptr},
	[__NR_amd64_accept4] = {"accept4", (void *)nullptr},
	[__NR_amd64_signalfd4] = {"signalfd4", (void *)nullptr},
	[__NR_amd64_eventfd2] = {"eventfd2", (void *)nullptr},
	[__NR_amd64_epoll_create1] = {"epoll_create1", (void *)nullptr},
	[__NR_amd64_dup3] = {"dup3", (void *)nullptr},
	[__NR_amd64_pipe2] = {"pipe2", (void *)linux_pipe2},
	[__NR_amd64_inotify_init1] = {"inotify_init1", (void *)nullptr},
	[__NR_amd64_preadv] = {"preadv", (void *)nullptr},
	[__NR_amd64_pwritev] = {"pwritev", (void *)nullptr},
	[__NR_amd64_rt_tgsigqueueinfo] = {"rt_tgsigqueueinfo", (void *)nullptr},
	[__NR_amd64_perf_event_open] = {"perf_event_open", (void *)nullptr},
	[__NR_amd64_recvmmsg] = {"recvmmsg", (void *)nullptr},
	[__NR_amd64_fanotify_init] = {"fanotify_init", (void *)nullptr},
	[__NR_amd64_fanotify_mark] = {"fanotify_mark", (void *)nullptr},
	[__NR_amd64_prlimit64] = {"prlimit64", (void *)linux_prlimit64},
	[__NR_amd64_name_to_handle_at] = {"name_to_handle_at", (void *)nullptr},
	[__NR_amd64_open_by_handle_at] = {"open_by_handle_at", (void *)nullptr},
	[__NR_amd64_clock_adjtime] = {"clock_adjtime", (void *)nullptr},
	[__NR_amd64_syncfs] = {"syncfs", (void *)nullptr},
	[__NR_amd64_sendmmsg] = {"sendmmsg", (void *)nullptr},
	[__NR_amd64_setns] = {"setns", (void *)nullptr},
	[__NR_amd64_getcpu] = {"getcpu", (void *)nullptr},
	[__NR_amd64_process_vm_readv] = {"process_vm_readv", (void *)nullptr},
	[__NR_amd64_process_vm_writev] = {"process_vm_writev", (void *)nullptr},
	[__NR_amd64_kcmp] = {"kcmp", (void *)nullptr},
	[__NR_amd64_finit_module] = {"finit_module", (void *)nullptr},
	[__NR_amd64_sched_setattr] = {"sched_setattr", (void *)nullptr},
	[__NR_amd64_sched_getattr] = {"sched_getattr", (void *)nullptr},
	[__NR_amd64_renameat2] = {"renameat2", (void *)nullptr},
	[__NR_amd64_seccomp] = {"seccomp", (void *)nullptr},
	[__NR_amd64_getrandom] = {"getrandom", (void *)linux_getrandom},
	[__NR_amd64_memfd_create] = {"memfd_create", (void *)nullptr},
	[__NR_amd64_kexec_file_load] = {"kexec_file_load", (void *)nullptr},
	[__NR_amd64_bpf] = {"bpf", (void *)nullptr},
	[__NR_amd64_execveat] = {"execveat", (void *)nullptr},
	[__NR_amd64_userfaultfd] = {"userfaultfd", (void *)nullptr},
	[__NR_amd64_membarrier] = {"membarrier", (void *)nullptr},
	[__NR_amd64_mlock2] = {"mlock2", (void *)nullptr},
	[__NR_amd64_copy_file_range] = {"copy_file_range", (void *)nullptr},
	[__NR_amd64_preadv2] = {"preadv2", (void *)nullptr},
	[__NR_amd64_pwritev2] = {"pwritev2", (void *)nullptr},
	[__NR_amd64_pkey_mprotect] = {"pkey_mprotect", (void *)nullptr},
	[__NR_amd64_pkey_alloc] = {"pkey_alloc", (void *)nullptr},
	[__NR_amd64_pkey_free] = {"pkey_free", (void *)nullptr},
	[__NR_amd64_statx] = {"statx", (void *)nullptr},
	[__NR_amd64_io_pgetevents] = {"io_pgetevents", (void *)nullptr},
	[__NR_amd64_rseq] = {"rseq", (void *)nullptr},
	[335] = {"reserved", (void *)nullptr},
	[336] = {"reserved", (void *)nullptr},
	[337] = {"reserved", (void *)nullptr},
	[338] = {"reserved", (void *)nullptr},
	[339] = {"reserved", (void *)nullptr},
	[340] = {"reserved", (void *)nullptr},
	[341] = {"reserved", (void *)nullptr},
	[342] = {"reserved", (void *)nullptr},
	[343] = {"reserved", (void *)nullptr},
	[344] = {"reserved", (void *)nullptr},
	[345] = {"reserved", (void *)nullptr},
	[346] = {"reserved", (void *)nullptr},
	[347] = {"reserved", (void *)nullptr},
	[348] = {"reserved", (void *)nullptr},
	[349] = {"reserved", (void *)nullptr},
	[350] = {"reserved", (void *)nullptr},
	[351] = {"reserved", (void *)nullptr},
	[352] = {"reserved", (void *)nullptr},
	[353] = {"reserved", (void *)nullptr},
	[354] = {"reserved", (void *)nullptr},
	[355] = {"reserved", (void *)nullptr},
	[356] = {"reserved", (void *)nullptr},
	[357] = {"reserved", (void *)nullptr},
	[358] = {"reserved", (void *)nullptr},
	[359] = {"reserved", (void *)nullptr},
	[360] = {"reserved", (void *)nullptr},
	[361] = {"reserved", (void *)nullptr},
	[362] = {"reserved", (void *)nullptr},
	[363] = {"reserved", (void *)nullptr},
	[364] = {"reserved", (void *)nullptr},
	[365] = {"reserved", (void *)nullptr},
	[366] = {"reserved", (void *)nullptr},
	[367] = {"reserved", (void *)nullptr},
	[368] = {"reserved", (void *)nullptr},
	[369] = {"reserved", (void *)nullptr},
	[370] = {"reserved", (void *)nullptr},
	[371] = {"reserved", (void *)nullptr},
	[372] = {"reserved", (void *)nullptr},
	[373] = {"reserved", (void *)nullptr},
	[374] = {"reserved", (void *)nullptr},
	[375] = {"reserved", (void *)nullptr},
	[376] = {"reserved", (void *)nullptr},
	[377] = {"reserved", (void *)nullptr},
	[378] = {"reserved", (void *)nullptr},
	[379] = {"reserved", (void *)nullptr},
	[380] = {"reserved", (void *)nullptr},
	[381] = {"reserved", (void *)nullptr},
	[382] = {"reserved", (void *)nullptr},
	[383] = {"reserved", (void *)nullptr},
	[384] = {"reserved", (void *)nullptr},
	[385] = {"reserved", (void *)nullptr},
	[386] = {"reserved", (void *)nullptr},
	[387] = {"reserved", (void *)nullptr},
	[388] = {"reserved", (void *)nullptr},
	[389] = {"reserved", (void *)nullptr},
	[390] = {"reserved", (void *)nullptr},
	[391] = {"reserved", (void *)nullptr},
	[392] = {"reserved", (void *)nullptr},
	[393] = {"reserved", (void *)nullptr},
	[394] = {"reserved", (void *)nullptr},
	[395] = {"reserved", (void *)nullptr},
	[396] = {"reserved", (void *)nullptr},
	[397] = {"reserved", (void *)nullptr},
	[398] = {"reserved", (void *)nullptr},
	[399] = {"reserved", (void *)nullptr},
	[400] = {"reserved", (void *)nullptr},
	[401] = {"reserved", (void *)nullptr},
	[402] = {"reserved", (void *)nullptr},
	[403] = {"reserved", (void *)nullptr},
	[404] = {"reserved", (void *)nullptr},
	[405] = {"reserved", (void *)nullptr},
	[406] = {"reserved", (void *)nullptr},
	[407] = {"reserved", (void *)nullptr},
	[408] = {"reserved", (void *)nullptr},
	[409] = {"reserved", (void *)nullptr},
	[410] = {"reserved", (void *)nullptr},
	[411] = {"reserved", (void *)nullptr},
	[412] = {"reserved", (void *)nullptr},
	[413] = {"reserved", (void *)nullptr},
	[414] = {"reserved", (void *)nullptr},
	[415] = {"reserved", (void *)nullptr},
	[416] = {"reserved", (void *)nullptr},
	[417] = {"reserved", (void *)nullptr},
	[418] = {"reserved", (void *)nullptr},
	[419] = {"reserved", (void *)nullptr},
	[420] = {"reserved", (void *)nullptr},
	[421] = {"reserved", (void *)nullptr},
	[422] = {"reserved", (void *)nullptr},
	[423] = {"reserved", (void *)nullptr},
	[__NR_amd64_pidfd_send_signal] = {"pidfd_send_signal", (void *)nullptr},
	[__NR_amd64_io_uring_setup] = {"io_uring_setup", (void *)nullptr},
	[__NR_amd64_io_uring_enter] = {"io_uring_enter", (void *)nullptr},
	[__NR_amd64_io_uring_register] = {"io_uring_register", (void *)nullptr},
	[__NR_amd64_open_tree] = {"open_tree", (void *)nullptr},
	[__NR_amd64_move_mount] = {"move_mount", (void *)nullptr},
	[__NR_amd64_fsopen] = {"fsopen", (void *)nullptr},
	[__NR_amd64_fsconfig] = {"fsconfig", (void *)nullptr},
	[__NR_amd64_fsmount] = {"fsmount", (void *)nullptr},
	[__NR_amd64_fspick] = {"fspick", (void *)nullptr},
	[__NR_amd64_pidfd_open] = {"pidfd_open", (void *)nullptr},
	[__NR_amd64_clone3] = {"clone3", (void *)nullptr},
	[__NR_amd64_close_range] = {"close_range", (void *)nullptr},
	[__NR_amd64_openat2] = {"openat2", (void *)nullptr},
	[__NR_amd64_pidfd_getfd] = {"pidfd_getfd", (void *)nullptr},
	[__NR_amd64_faccessat2] = {"faccessat2", (void *)nullptr},
	[__NR_amd64_process_madvise] = {"process_madvise", (void *)nullptr},
	[__NR_amd64_epoll_pwait2] = {"epoll_pwait2", (void *)nullptr},
	[__NR_amd64_mount_setattr] = {"mount_setattr", (void *)nullptr},
	[443] = {"reserved", (void *)nullptr},
	[__NR_amd64_landlock_create_ruleset] = {"landlock_create_ruleset", (void *)nullptr},
	[__NR_amd64_landlock_add_rule] = {"landlock_add_rule", (void *)nullptr},
	[__NR_amd64_landlock_restrict_self] = {"landlock_restrict_self", (void *)nullptr},
};

static SyscallData LinuxSyscallsTableI386[] = {
	[__NR_i386_restart_syscall] = {"restart_syscall", (void *)nullptr},
	[__NR_i386_exit] = {"exit", (void *)linux_exit},
	[__NR_i386_fork] = {"fork", (void *)linux_fork},
	[__NR_i386_read] = {"read", (void *)linux_read},
	[__NR_i386_write] = {"write", (void *)linux_write},
	[__NR_i386_open] = {"open", (void *)linux_open},
	[__NR_i386_close] = {"close", (void *)linux_close},
	[__NR_i386_waitpid] = {"waitpid", (void *)nullptr},
	[__NR_i386_creat] = {"creat", (void *)linux_creat},
	[__NR_i386_link] = {"link", (void *)nullptr},
	[__NR_i386_unlink] = {"unlink", (void *)nullptr},
	[__NR_i386_execve] = {"execve", (void *)linux_execve},
	[__NR_i386_chdir] = {"chdir", (void *)linux_chdir},
	[__NR_i386_time] = {"time", (void *)nullptr},
	[__NR_i386_mknod] = {"mknod", (void *)nullptr},
	[__NR_i386_chmod] = {"chmod", (void *)linux_chmod},
	[__NR_i386_lchown] = {"lchown", (void *)nullptr},
	[__NR_i386_break] = {"break", (void *)nullptr},
	[__NR_i386_oldstat] = {"oldstat", (void *)nullptr},
	[__NR_i386_lseek] = {"lseek", (void *)linux_lseek},
	[__NR_i386_getpid] = {"getpid", (void *)linux_getpid},
	[__NR_i386_mount] = {"mount", (void *)nullptr},
	[__NR_i386_umount] = {"umount", (void *)nullptr},
	[__NR_i386_setuid] = {"setuid", (void *)nullptr},
	[__NR_i386_getuid] = {"getuid", (void *)nullptr},
	[__NR_i386_stime] = {"stime", (void *)nullptr},
	[__NR_i386_ptrace] = {"ptrace", (void *)nullptr},
	[__NR_i386_alarm] = {"alarm", (void *)nullptr},
	[__NR_i386_oldfstat] = {"oldfstat", (void *)nullptr},
	[__NR_i386_pause] = {"pause", (void *)nullptr},
	[__NR_i386_utime] = {"utime", (void *)nullptr},
	[__NR_i386_stty] = {"stty", (void *)nullptr},
	[__NR_i386_gtty] = {"gtty", (void *)nullptr},
	[__NR_i386_access] = {"access", (void *)linux_access},
	[__NR_i386_nice] = {"nice", (void *)nullptr},
	[__NR_i386_ftime] = {"ftime", (void *)nullptr},
	[__NR_i386_sync] = {"sync", (void *)nullptr},
	[__NR_i386_kill] = {"kill", (void *)linux_kill},
	[__NR_i386_rename] = {"rename", (void *)nullptr},
	[__NR_i386_mkdir] = {"mkdir", (void *)linux_mkdir},
	[__NR_i386_rmdir] = {"rmdir", (void *)nullptr},
	[__NR_i386_dup] = {"dup", (void *)linux_dup},
	[__NR_i386_pipe] = {"pipe", (void *)nullptr},
	[__NR_i386_times] = {"times", (void *)nullptr},
	[__NR_i386_prof] = {"prof", (void *)nullptr},
	[__NR_i386_brk] = {"brk", (void *)linux_brk},
	[__NR_i386_setgid] = {"setgid", (void *)nullptr},
	[__NR_i386_getgid] = {"getgid", (void *)linux_getgid},
	[__NR_i386_signal] = {"signal", (void *)nullptr},
	[__NR_i386_geteuid] = {"geteuid", (void *)linux_geteuid},
	[__NR_i386_getegid] = {"getegid", (void *)linux_getegid},
	[__NR_i386_acct] = {"acct", (void *)nullptr},
	[__NR_i386_umount2] = {"umount2", (void *)nullptr},
	[__NR_i386_lock] = {"lock", (void *)nullptr},
	[__NR_i386_ioctl] = {"ioctl", (void *)linux_ioctl},
	[__NR_i386_fcntl] = {"fcntl", (void *)linux_fcntl},
	[__NR_i386_mpx] = {"mpx", (void *)nullptr},
	[__NR_i386_setpgid] = {"setpgid", (void *)linux_setpgid},
	[__NR_i386_ulimit] = {"ulimit", (void *)nullptr},
	[__NR_i386_oldolduname] = {"oldolduname", (void *)nullptr},
	[__NR_i386_umask] = {"umask", (void *)linux_umask},
	[__NR_i386_chroot] = {"chroot", (void *)nullptr},
	[__NR_i386_ustat] = {"ustat", (void *)nullptr},
	[__NR_i386_dup2] = {"dup2", (void *)linux_dup2},
	[__NR_i386_getppid] = {"getppid", (void *)linux_getppid},
	[__NR_i386_getpgrp] = {"getpgrp", (void *)nullptr},
	[__NR_i386_setsid] = {"setsid", (void *)nullptr},
	[__NR_i386_sigaction] = {"sigaction", (void *)nullptr},
	[__NR_i386_sgetmask] = {"sgetmask", (void *)nullptr},
	[__NR_i386_ssetmask] = {"ssetmask", (void *)nullptr},
	[__NR_i386_setreuid] = {"setreuid", (void *)nullptr},
	[__NR_i386_setregid] = {"setregid", (void *)nullptr},
	[__NR_i386_sigsuspend] = {"sigsuspend", (void *)nullptr},
	[__NR_i386_sigpending] = {"sigpending", (void *)nullptr},
	[__NR_i386_sethostname] = {"sethostname", (void *)nullptr},
	[__NR_i386_setrlimit] = {"setrlimit", (void *)nullptr},
	[__NR_i386_getrlimit] = {"getrlimit", (void *)nullptr},
	[__NR_i386_getrusage] = {"getrusage", (void *)linux_getrusage},
	[__NR_i386_gettimeofday_time32] = {"gettimeofday_time32", (void *)nullptr},
	[__NR_i386_settimeofday_time32] = {"settimeofday_time32", (void *)nullptr},
	[__NR_i386_getgroups] = {"getgroups", (void *)nullptr},
	[__NR_i386_setgroups] = {"setgroups", (void *)nullptr},
	[__NR_i386_select] = {"select", (void *)nullptr},
	[__NR_i386_symlink] = {"symlink", (void *)nullptr},
	[__NR_i386_oldlstat] = {"oldlstat", (void *)nullptr},
	[__NR_i386_readlink] = {"readlink", (void *)linux_readlink},
	[__NR_i386_uselib] = {"uselib", (void *)nullptr},
	[__NR_i386_swapon] = {"swapon", (void *)nullptr},
	[__NR_i386_reboot] = {"reboot", (void *)linux_reboot},
	[__NR_i386_readdir] = {"readdir", (void *)nullptr},
	[__NR_i386_mmap] = {"mmap", (void *)linux_mmap},
	[__NR_i386_munmap] = {"munmap", (void *)linux_munmap},
	[__NR_i386_truncate] = {"truncate", (void *)nullptr},
	[__NR_i386_ftruncate] = {"ftruncate", (void *)nullptr},
	[__NR_i386_fchmod] = {"fchmod", (void *)linux_fchmod},
	[__NR_i386_fchown] = {"fchown", (void *)nullptr},
	[__NR_i386_getpriority] = {"getpriority", (void *)nullptr},
	[__NR_i386_setpriority] = {"setpriority", (void *)nullptr},
	[__NR_i386_profil] = {"profil", (void *)nullptr},
	[__NR_i386_statfs] = {"statfs", (void *)nullptr},
	[__NR_i386_fstatfs] = {"fstatfs", (void *)nullptr},
	[__NR_i386_ioperm] = {"ioperm", (void *)nullptr},
	[__NR_i386_socketcall] = {"socketcall", (void *)nullptr},
	[__NR_i386_syslog] = {"syslog", (void *)linux_syslog},
	[__NR_i386_setitimer] = {"setitimer", (void *)linux_setitimer},
	[__NR_i386_getitimer] = {"getitimer", (void *)nullptr},
	[__NR_i386_stat] = {"stat", (void *)linux_stat},
	[__NR_i386_lstat] = {"lstat", (void *)linux_lstat},
	[__NR_i386_fstat] = {"fstat", (void *)linux_fstat},
	[__NR_i386_olduname] = {"olduname", (void *)nullptr},
	[__NR_i386_iopl] = {"iopl", (void *)nullptr},
	[__NR_i386_vhangup] = {"vhangup", (void *)nullptr},
	[__NR_i386_idle] = {"idle", (void *)nullptr},
	[__NR_i386_vm86old] = {"vm86old", (void *)nullptr},
	[__NR_i386_wait4] = {"wait4", (void *)linux_wait4},
	[__NR_i386_swapoff] = {"swapoff", (void *)nullptr},
	[__NR_i386_sysinfo] = {"sysinfo", (void *)linux_sysinfo},
	[__NR_i386_ipc] = {"ipc", (void *)nullptr},
	[__NR_i386_fsync] = {"fsync", (void *)nullptr},
	[__NR_i386_sigreturn] = {"sigreturn", (void *)nullptr},
	[__NR_i386_clone] = {"clone", (void *)nullptr},
	[__NR_i386_setdomainname] = {"setdomainname", (void *)nullptr},
	[__NR_i386_uname] = {"uname", (void *)linux_uname},
	[__NR_i386_modify_ldt] = {"modify_ldt", (void *)nullptr},
	[__NR_i386_adjtimex] = {"adjtimex", (void *)nullptr},
	[__NR_i386_mprotect] = {"mprotect", (void *)linux_mprotect},
	[__NR_i386_sigprocmask] = {"sigprocmask", (void *)nullptr},
	[__NR_i386_create_module] = {"create_module", (void *)nullptr},
	[__NR_i386_init_module] = {"init_module", (void *)nullptr},
	[__NR_i386_delete_module] = {"delete_module", (void *)nullptr},
	[__NR_i386_get_kernel_syms] = {"get_kernel_syms", (void *)nullptr},
	[__NR_i386_quotactl] = {"quotactl", (void *)nullptr},
	[__NR_i386_getpgid] = {"getpgid", (void *)linux_getpgid},
	[__NR_i386_fchdir] = {"fchdir", (void *)linux_fchdir},
	[__NR_i386_bdflush] = {"bdflush", (void *)nullptr},
	[__NR_i386_sysfs] = {"sysfs", (void *)nullptr},
	[__NR_i386_personality] = {"personality", (void *)nullptr},
	[__NR_i386_afs_syscall] = {"afs_syscall", (void *)nullptr},
	[__NR_i386_setfsuid] = {"setfsuid", (void *)nullptr},
	[__NR_i386_setfsgid] = {"setfsgid", (void *)nullptr},
	[__NR_i386__llseek] = {"_llseek", (void *)nullptr},
	[__NR_i386_getdents] = {"getdents", (void *)nullptr},
	[__NR_i386__newselect] = {"_newselect", (void *)nullptr},
	[__NR_i386_flock] = {"flock", (void *)nullptr},
	[__NR_i386_msync] = {"msync", (void *)nullptr},
	[__NR_i386_readv] = {"readv", (void *)linux_readv},
	[__NR_i386_writev] = {"writev", (void *)linux_writev},
	[__NR_i386_getsid] = {"getsid", (void *)nullptr},
	[__NR_i386_fdatasync] = {"fdatasync", (void *)nullptr},
	[__NR_i386__sysctl] = {"_sysctl", (void *)nullptr},
	[__NR_i386_mlock] = {"mlock", (void *)nullptr},
	[__NR_i386_munlock] = {"munlock", (void *)nullptr},
	[__NR_i386_mlockall] = {"mlockall", (void *)nullptr},
	[__NR_i386_munlockall] = {"munlockall", (void *)nullptr},
	[__NR_i386_sched_setparam] = {"sched_setparam", (void *)nullptr},
	[__NR_i386_sched_getparam] = {"sched_getparam", (void *)nullptr},
	[__NR_i386_sched_setscheduler] = {"sched_setscheduler", (void *)nullptr},
	[__NR_i386_sched_getscheduler] = {"sched_getscheduler", (void *)nullptr},
	[__NR_i386_sched_yield] = {"sched_yield", (void *)nullptr},
	[__NR_i386_sched_get_priority_max] = {"sched_get_priority_max", (void *)nullptr},
	[__NR_i386_sched_get_priority_min] = {"sched_get_priority_min", (void *)nullptr},
	[__NR_i386_sched_rr_get_interval] = {"sched_rr_get_interval", (void *)nullptr},
	[__NR_i386_nanosleep] = {"nanosleep", (void *)nullptr},
	[__NR_i386_mremap] = {"mremap", (void *)nullptr},
	[__NR_i386_setresuid] = {"setresuid", (void *)nullptr},
	[__NR_i386_getresuid] = {"getresuid", (void *)nullptr},
	[__NR_i386_vm86] = {"vm86", (void *)nullptr},
	[__NR_i386_query_module] = {"query_module", (void *)nullptr},
	[__NR_i386_poll] = {"poll", (void *)linux_poll},
	[__NR_i386_nfsservctl] = {"nfsservctl", (void *)nullptr},
	[__NR_i386_setresgid] = {"setresgid", (void *)nullptr},
	[__NR_i386_getresgid] = {"getresgid", (void *)nullptr},
	[__NR_i386_prctl] = {"prctl", (void *)nullptr},
	[__NR_i386_rt_sigreturn] = {"rt_sigreturn", (void *)linux_sigreturn},
	[__NR_i386_rt_sigaction] = {"rt_sigaction", (void *)nullptr},
	[__NR_i386_rt_sigprocmask] = {"rt_sigprocmask", (void *)linux_sigprocmask},
	[__NR_i386_rt_sigpending] = {"rt_sigpending", (void *)nullptr},
	[__NR_i386_rt_sigtimedwait] = {"rt_sigtimedwait", (void *)nullptr},
	[__NR_i386_rt_sigqueueinfo] = {"rt_sigqueueinfo", (void *)nullptr},
	[__NR_i386_rt_sigsuspend] = {"rt_sigsuspend", (void *)nullptr},
	[__NR_i386_pread64] = {"pread64", (void *)linux_pread64},
	[__NR_i386_pwrite64] = {"pwrite64", (void *)linux_pwrite64},
	[__NR_i386_chown] = {"chown", (void *)nullptr},
	[__NR_i386_getcwd] = {"getcwd", (void *)linux_getcwd},
	[__NR_i386_capget] = {"capget", (void *)nullptr},
	[__NR_i386_capset] = {"capset", (void *)nullptr},
	[__NR_i386_sigaltstack] = {"sigaltstack", (void *)nullptr},
	[__NR_i386_sendfile] = {"sendfile", (void *)nullptr},
	[__NR_i386_getpmsg] = {"getpmsg", (void *)nullptr},
	[__NR_i386_putpmsg] = {"putpmsg", (void *)nullptr},
	[__NR_i386_vfork] = {"vfork", (void *)linux_vfork},
	[__NR_i386_ugetrlimit] = {"ugetrlimit", (void *)nullptr},
	[__NR_i386_mmap2] = {"mmap2", (void *)nullptr},
	[__NR_i386_truncate64] = {"truncate64", (void *)nullptr},
	[__NR_i386_ftruncate64] = {"ftruncate64", (void *)nullptr},
	[__NR_i386_stat64] = {"stat64", (void *)nullptr},
	[__NR_i386_lstat64] = {"lstat64", (void *)nullptr},
	[__NR_i386_fstat64] = {"fstat64", (void *)nullptr},
	[__NR_i386_lchown32] = {"lchown32", (void *)nullptr},
	[__NR_i386_getuid32] = {"getuid32", (void *)nullptr},
	[__NR_i386_getgid32] = {"getgid32", (void *)nullptr},
	[__NR_i386_geteuid32] = {"geteuid32", (void *)nullptr},
	[__NR_i386_getegid32] = {"getegid32", (void *)nullptr},
	[__NR_i386_setreuid32] = {"setreuid32", (void *)nullptr},
	[__NR_i386_setregid32] = {"setregid32", (void *)nullptr},
	[__NR_i386_getgroups32] = {"getgroups32", (void *)nullptr},
	[__NR_i386_setgroups32] = {"setgroups32", (void *)nullptr},
	[__NR_i386_fchown32] = {"fchown32", (void *)nullptr},
	[__NR_i386_setresuid32] = {"setresuid32", (void *)nullptr},
	[__NR_i386_getresuid32] = {"getresuid32", (void *)nullptr},
	[__NR_i386_setresgid32] = {"setresgid32", (void *)nullptr},
	[__NR_i386_getresgid32] = {"getresgid32", (void *)nullptr},
	[__NR_i386_chown32] = {"chown32", (void *)nullptr},
	[__NR_i386_setuid32] = {"setuid32", (void *)nullptr},
	[__NR_i386_setgid32] = {"setgid32", (void *)nullptr},
	[__NR_i386_setfsuid32] = {"setfsuid32", (void *)nullptr},
	[__NR_i386_setfsgid32] = {"setfsgid32", (void *)nullptr},
	[__NR_i386_pivot_root] = {"pivot_root", (void *)nullptr},
	[__NR_i386_mincore] = {"mincore", (void *)nullptr},
	[__NR_i386_madvise] = {"madvise", (void *)linux_madvise},
	[__NR_i386_getdents64] = {"getdents64", (void *)linux_getdents64},
	[__NR_i386_fcntl64] = {"fcntl64", (void *)nullptr},
	[222] = {"reserved", (void *)nullptr},
	[223] = {"reserved", (void *)nullptr},
	[__NR_i386_gettid] = {"gettid", (void *)linux_gettid},
	[__NR_i386_readahead] = {"readahead", (void *)nullptr},
	[__NR_i386_setxattr] = {"setxattr", (void *)nullptr},
	[__NR_i386_lsetxattr] = {"lsetxattr", (void *)nullptr},
	[__NR_i386_fsetxattr] = {"fsetxattr", (void *)nullptr},
	[__NR_i386_getxattr] = {"getxattr", (void *)nullptr},
	[__NR_i386_lgetxattr] = {"lgetxattr", (void *)nullptr},
	[__NR_i386_fgetxattr] = {"fgetxattr", (void *)nullptr},
	[__NR_i386_listxattr] = {"listxattr", (void *)nullptr},
	[__NR_i386_llistxattr] = {"llistxattr", (void *)nullptr},
	[__NR_i386_flistxattr] = {"flistxattr", (void *)nullptr},
	[__NR_i386_removexattr] = {"removexattr", (void *)nullptr},
	[__NR_i386_lremovexattr] = {"lremovexattr", (void *)nullptr},
	[__NR_i386_fremovexattr] = {"fremovexattr", (void *)nullptr},
	[__NR_i386_tkill] = {"tkill", (void *)linux_tkill},
	[__NR_i386_sendfile64] = {"sendfile64", (void *)nullptr},
	[__NR_i386_futex] = {"futex", (void *)nullptr},
	[__NR_i386_sched_setaffinity] = {"sched_setaffinity", (void *)linux_sched_setaffinity},
	[__NR_i386_sched_getaffinity] = {"sched_getaffinity", (void *)linux_sched_getaffinity},
	[__NR_i386_set_thread_area] = {"set_thread_area", (void *)nullptr},
	[__NR_i386_get_thread_area] = {"get_thread_area", (void *)nullptr},
	[__NR_i386_io_setup] = {"io_setup", (void *)nullptr},
	[__NR_i386_io_destroy] = {"io_destroy", (void *)nullptr},
	[__NR_i386_io_getevents] = {"io_getevents", (void *)nullptr},
	[__NR_i386_io_submit] = {"io_submit", (void *)nullptr},
	[__NR_i386_io_cancel] = {"io_cancel", (void *)nullptr},
	[__NR_i386_fadvise64] = {"fadvise64", (void *)nullptr},
	[__NR_i386_set_zone_reclaim] = {"set_zone_reclaim", (void *)nullptr},
	[__NR_i386_exit_group] = {"exit_group", (void *)linux_exit_group},
	[__NR_i386_lookup_dcookie] = {"lookup_dcookie", (void *)nullptr},
	[__NR_i386_epoll_create] = {"epoll_create", (void *)nullptr},
	[__NR_i386_epoll_ctl] = {"epoll_ctl", (void *)nullptr},
	[__NR_i386_epoll_wait] = {"epoll_wait", (void *)nullptr},
	[__NR_i386_remap_file_pages] = {"remap_file_pages", (void *)nullptr},
	[__NR_i386_set_tid_address] = {"set_tid_address", (void *)linux_set_tid_address},
	[__NR_i386_timer_create] = {"timer_create", (void *)nullptr},
	[__NR_i386_timer_settime32] = {"timer_settime32", (void *)nullptr},
	[__NR_i386_timer_gettime32] = {"timer_gettime32", (void *)nullptr},
	[__NR_i386_timer_getoverrun] = {"timer_getoverrun", (void *)nullptr},
	[__NR_i386_timer_delete] = {"timer_delete", (void *)nullptr},
	[__NR_i386_clock_settime32] = {"clock_settime32", (void *)nullptr},
	[__NR_i386_clock_gettime32] = {"clock_gettime32", (void *)nullptr},
	[__NR_i386_clock_getres_time32] = {"clock_getres_time32", (void *)nullptr},
	[__NR_i386_clock_nanosleep_time32] = {"clock_nanosleep_time32", (void *)nullptr},
	[__NR_i386_statfs64] = {"statfs64", (void *)nullptr},
	[__NR_i386_fstatfs64] = {"fstatfs64", (void *)nullptr},
	[__NR_i386_tgkill] = {"tgkill", (void *)linux_tgkill},
	[__NR_i386_utimes] = {"utimes", (void *)nullptr},
	[__NR_i386_fadvise64_64] = {"fadvise64_64", (void *)nullptr},
	[__NR_i386_vserver] = {"vserver", (void *)nullptr},
	[__NR_i386_mbind] = {"mbind", (void *)nullptr},
	[__NR_i386_get_mempolicy] = {"get_mempolicy", (void *)nullptr},
	[__NR_i386_set_mempolicy] = {"set_mempolicy", (void *)nullptr},
	[__NR_i386_mq_open] = {"mq_open", (void *)nullptr},
	[__NR_i386_mq_unlink] = {"mq_unlink", (void *)nullptr},
	[__NR_i386_mq_timedsend] = {"mq_timedsend", (void *)nullptr},
	[__NR_i386_mq_timedreceive] = {"mq_timedreceive", (void *)nullptr},
	[__NR_i386_mq_notify] = {"mq_notify", (void *)nullptr},
	[__NR_i386_mq_getsetattr] = {"mq_getsetattr", (void *)nullptr},
	[__NR_i386_kexec_load] = {"kexec_load", (void *)nullptr},
	[__NR_i386_waitid] = {"waitid", (void *)nullptr},
	[__NR_i386_sys_setaltroot] = {"sys_setaltroot", (void *)nullptr},
	[__NR_i386_add_key] = {"add_key", (void *)nullptr},
	[__NR_i386_request_key] = {"request_key", (void *)nullptr},
	[__NR_i386_keyctl] = {"keyctl", (void *)nullptr},
	[__NR_i386_ioprio_set] = {"ioprio_set", (void *)nullptr},
	[__NR_i386_ioprio_get] = {"ioprio_get", (void *)nullptr},
	[__NR_i386_inotify_init] = {"inotify_init", (void *)nullptr},
	[__NR_i386_inotify_add_watch] = {"inotify_add_watch", (void *)nullptr},
	[__NR_i386_inotify_rm_watch] = {"inotify_rm_watch", (void *)nullptr},
	[__NR_i386_migrate_pages] = {"migrate_pages", (void *)nullptr},
	[__NR_i386_openat] = {"openat", (void *)linux_openat},
	[__NR_i386_mkdirat] = {"mkdirat", (void *)nullptr},
	[__NR_i386_mknodat] = {"mknodat", (void *)nullptr},
	[__NR_i386_fchownat] = {"fchownat", (void *)nullptr},
	[__NR_i386_futimesat] = {"futimesat", (void *)nullptr},
	[__NR_i386_fstatat64] = {"fstatat64", (void *)nullptr},
	[__NR_i386_unlinkat] = {"unlinkat", (void *)nullptr},
	[__NR_i386_renameat] = {"renameat", (void *)nullptr},
	[__NR_i386_linkat] = {"linkat", (void *)nullptr},
	[__NR_i386_symlinkat] = {"symlinkat", (void *)nullptr},
	[__NR_i386_readlinkat] = {"readlinkat", (void *)nullptr},
	[__NR_i386_fchmodat] = {"fchmodat", (void *)nullptr},
	[__NR_i386_faccessat] = {"faccessat", (void *)nullptr},
	[__NR_i386_pselect6] = {"pselect6", (void *)nullptr},
	[__NR_i386_ppoll] = {"ppoll", (void *)nullptr},
	[__NR_i386_unshare] = {"unshare", (void *)nullptr},
	[__NR_i386_set_robust_list] = {"set_robust_list", (void *)nullptr},
	[__NR_i386_get_robust_list] = {"get_robust_list", (void *)nullptr},
	[__NR_i386_splice] = {"splice", (void *)nullptr},
	[__NR_i386_sync_file_range] = {"sync_file_range", (void *)nullptr},
	[__NR_i386_tee] = {"tee", (void *)nullptr},
	[__NR_i386_vmsplice] = {"vmsplice", (void *)nullptr},
	[__NR_i386_move_pages] = {"move_pages", (void *)nullptr},
	[__NR_i386_getcpu] = {"getcpu", (void *)nullptr},
	[__NR_i386_epoll_pwait] = {"epoll_pwait", (void *)nullptr},
	[__NR_i386_utimensat] = {"utimensat", (void *)nullptr},
	[__NR_i386_signalfd] = {"signalfd", (void *)nullptr},
	[__NR_i386_timerfd_create] = {"timerfd_create", (void *)nullptr},
	[__NR_i386_eventfd] = {"eventfd", (void *)nullptr},
	[__NR_i386_fallocate] = {"fallocate", (void *)nullptr},
	[__NR_i386_timerfd_settime32] = {"timerfd_settime32", (void *)nullptr},
	[__NR_i386_timerfd_gettime32] = {"timerfd_gettime32", (void *)nullptr},
	[__NR_i386_signalfd4] = {"signalfd4", (void *)nullptr},
	[__NR_i386_eventfd2] = {"eventfd2", (void *)nullptr},
	[__NR_i386_epoll_create1] = {"epoll_create1", (void *)nullptr},
	[__NR_i386_dup3] = {"dup3", (void *)nullptr},
	[__NR_i386_pipe2] = {"pipe2", (void *)nullptr},
	[__NR_i386_inotify_init1] = {"inotify_init1", (void *)nullptr},
	[__NR_i386_preadv] = {"preadv", (void *)nullptr},
	[__NR_i386_pwritev] = {"pwritev", (void *)nullptr},
	[__NR_i386_rt_tgsigqueueinfo] = {"rt_tgsigqueueinfo", (void *)nullptr},
	[__NR_i386_perf_event_open] = {"perf_event_open", (void *)nullptr},
	[__NR_i386_recvmmsg] = {"recvmmsg", (void *)nullptr},
	[__NR_i386_fanotify_init] = {"fanotify_init", (void *)nullptr},
	[__NR_i386_fanotify_mark] = {"fanotify_mark", (void *)nullptr},
	[__NR_i386_prlimit64] = {"prlimit64", (void *)nullptr},
	[__NR_i386_name_to_handle_at] = {"name_to_handle_at", (void *)nullptr},
	[__NR_i386_open_by_handle_at] = {"open_by_handle_at", (void *)nullptr},
	[__NR_i386_clock_adjtime] = {"clock_adjtime", (void *)nullptr},
	[__NR_i386_syncfs] = {"syncfs", (void *)nullptr},
	[__NR_i386_sendmmsg] = {"sendmmsg", (void *)nullptr},
	[__NR_i386_setns] = {"setns", (void *)nullptr},
	[__NR_i386_process_vm_readv] = {"process_vm_readv", (void *)nullptr},
	[__NR_i386_process_vm_writev] = {"process_vm_writev", (void *)nullptr},
	[__NR_i386_kcmp] = {"kcmp", (void *)nullptr},
	[__NR_i386_finit_module] = {"finit_module", (void *)nullptr},
	[__NR_i386_sched_setattr] = {"sched_setattr", (void *)nullptr},
	[__NR_i386_sched_getattr] = {"sched_getattr", (void *)nullptr},
	[__NR_i386_renameat2] = {"renameat2", (void *)nullptr},
	[__NR_i386_seccomp] = {"seccomp", (void *)nullptr},
	[__NR_i386_getrandom] = {"getrandom", (void *)linux_getrandom},
	[__NR_i386_memfd_create] = {"memfd_create", (void *)nullptr},
	[__NR_i386_bpf] = {"bpf", (void *)nullptr},
	[__NR_i386_execveat] = {"execveat", (void *)nullptr},
	[__NR_i386_socket] = {"socket", (void *)nullptr},
	[__NR_i386_socketpair] = {"socketpair", (void *)nullptr},
	[__NR_i386_bind] = {"bind", (void *)nullptr},
	[__NR_i386_connect] = {"connect", (void *)nullptr},
	[__NR_i386_listen] = {"listen", (void *)nullptr},
	[__NR_i386_accept4] = {"accept4", (void *)nullptr},
	[__NR_i386_getsockopt] = {"getsockopt", (void *)nullptr},
	[__NR_i386_setsockopt] = {"setsockopt", (void *)nullptr},
	[__NR_i386_getsockname] = {"getsockname", (void *)nullptr},
	[__NR_i386_getpeername] = {"getpeername", (void *)nullptr},
	[__NR_i386_sendto] = {"sendto", (void *)nullptr},
	[__NR_i386_sendmsg] = {"sendmsg", (void *)nullptr},
	[__NR_i386_recvfrom] = {"recvfrom", (void *)nullptr},
	[__NR_i386_recvmsg] = {"recvmsg", (void *)nullptr},
	[__NR_i386_shutdown] = {"shutdown", (void *)linux_shutdown},
	[__NR_i386_userfaultfd] = {"userfaultfd", (void *)nullptr},
	[__NR_i386_membarrier] = {"membarrier", (void *)nullptr},
	[__NR_i386_mlock2] = {"mlock2", (void *)nullptr},
	[__NR_i386_copy_file_range] = {"copy_file_range", (void *)nullptr},
	[__NR_i386_preadv2] = {"preadv2", (void *)nullptr},
	[__NR_i386_pwritev2] = {"pwritev2", (void *)nullptr},
	[__NR_i386_pkey_mprotect] = {"pkey_mprotect", (void *)nullptr},
	[__NR_i386_pkey_alloc] = {"pkey_alloc", (void *)nullptr},
	[__NR_i386_pkey_free] = {"pkey_free", (void *)nullptr},
	[__NR_i386_statx] = {"statx", (void *)nullptr},
	[__NR_i386_arch_prctl] = {"arch_prctl", (void *)linux_arch_prctl},
	[__NR_i386_io_pgetevents] = {"io_pgetevents", (void *)nullptr},
	[__NR_i386_rseq] = {"rseq", (void *)nullptr},
	[387] = {"reserved", (void *)nullptr},
	[388] = {"reserved", (void *)nullptr},
	[389] = {"reserved", (void *)nullptr},
	[390] = {"reserved", (void *)nullptr},
	[391] = {"reserved", (void *)nullptr},
	[392] = {"reserved", (void *)nullptr},
	[__NR_i386_semget] = {"semget", (void *)nullptr},
	[__NR_i386_semctl] = {"semctl", (void *)nullptr},
	[__NR_i386_shmget] = {"shmget", (void *)nullptr},
	[__NR_i386_shmctl] = {"shmctl", (void *)nullptr},
	[__NR_i386_shmat] = {"shmat", (void *)nullptr},
	[__NR_i386_shmdt] = {"shmdt", (void *)nullptr},
	[__NR_i386_msgget] = {"msgget", (void *)nullptr},
	[__NR_i386_msgsnd] = {"msgsnd", (void *)nullptr},
	[__NR_i386_msgrcv] = {"msgrcv", (void *)nullptr},
	[__NR_i386_msgctl] = {"msgctl", (void *)nullptr},
	[__NR_i386_clock_gettime64] = {"clock_gettime64", (void *)nullptr},
	[__NR_i386_clock_settime64] = {"clock_settime64", (void *)nullptr},
	[__NR_i386_clock_adjtime64] = {"clock_adjtime64", (void *)nullptr},
	[__NR_i386_clock_getres_time64] = {"clock_getres_time64", (void *)nullptr},
	[__NR_i386_clock_nanosleep_time64] = {"clock_nanosleep_time64", (void *)nullptr},
	[__NR_i386_timer_gettime64] = {"timer_gettime64", (void *)nullptr},
	[__NR_i386_timer_settime64] = {"timer_settime64", (void *)nullptr},
	[__NR_i386_timerfd_gettime64] = {"timerfd_gettime64", (void *)nullptr},
	[__NR_i386_timerfd_settime64] = {"timerfd_settime64", (void *)nullptr},
	[__NR_i386_utimensat_time64] = {"utimensat_time64", (void *)nullptr},
	[__NR_i386_pselect6_time64] = {"pselect6_time64", (void *)nullptr},
	[__NR_i386_ppoll_time64] = {"ppoll_time64", (void *)nullptr},
	[415] = {"reserved", (void *)nullptr},
	[__NR_i386_io_pgetevents_time64] = {"io_pgetevents_time64", (void *)nullptr},
	[__NR_i386_recvmmsg_time64] = {"recvmmsg_time64", (void *)nullptr},
	[__NR_i386_mq_timedsend_time64] = {"mq_timedsend_time64", (void *)nullptr},
	[__NR_i386_mq_timedreceive_time64] = {"mq_timedreceive_time64", (void *)nullptr},
	[__NR_i386_semtimedop_time64] = {"semtimedop_time64", (void *)nullptr},
	[__NR_i386_rt_sigtimedwait_time64] = {"rt_sigtimedwait_time64", (void *)nullptr},
	[__NR_i386_futex_time64] = {"futex_time64", (void *)nullptr},
	[__NR_i386_sched_rr_get_interval_time64] = {"sched_rr_get_interval_time64", (void *)nullptr},
	[__NR_i386_pidfd_send_signal] = {"pidfd_send_signal", (void *)nullptr},
	[__NR_i386_io_uring_setup] = {"io_uring_setup", (void *)nullptr},
	[__NR_i386_io_uring_enter] = {"io_uring_enter", (void *)nullptr},
	[__NR_i386_io_uring_register] = {"io_uring_register", (void *)nullptr},
	[__NR_i386_open_tree] = {"open_tree", (void *)nullptr},
	[__NR_i386_move_mount] = {"move_mount", (void *)nullptr},
	[__NR_i386_fsopen] = {"fsopen", (void *)nullptr},
	[__NR_i386_fsconfig] = {"fsconfig", (void *)nullptr},
	[__NR_i386_fsmount] = {"fsmount", (void *)nullptr},
	[__NR_i386_fspick] = {"fspick", (void *)nullptr},
	[__NR_i386_pidfd_open] = {"pidfd_open", (void *)nullptr},
	[__NR_i386_clone3] = {"clone3", (void *)nullptr},
	[__NR_i386_close_range] = {"close_range", (void *)nullptr},
	[__NR_i386_openat2] = {"openat2", (void *)nullptr},
	[__NR_i386_pidfd_getfd] = {"pidfd_getfd", (void *)nullptr},
	[__NR_i386_faccessat2] = {"faccessat2", (void *)nullptr},
	[__NR_i386_process_madvise] = {"process_madvise", (void *)nullptr},
	[__NR_i386_epoll_pwait2] = {"epoll_pwait2", (void *)nullptr},
	[__NR_i386_mount_setattr] = {"mount_setattr", (void *)nullptr},
	[443] = {"reserved", (void *)nullptr},
	[__NR_i386_landlock_create_ruleset] = {"landlock_create_ruleset", (void *)nullptr},
	[__NR_i386_landlock_add_rule] = {"landlock_add_rule", (void *)nullptr},
	[__NR_i386_landlock_restrict_self] = {"landlock_restrict_self", (void *)nullptr},
};

uintptr_t HandleLinuxSyscalls(SyscallsFrame *Frame)
{
#if defined(__amd64__)
	if (Frame->ax > sizeof(LinuxSyscallsTableAMD64) / sizeof(SyscallData))
	{
		fixme("Syscall %d not implemented",
			  Frame->ax);
		return -linux_ENOSYS;
	}

	SyscallData Syscall = LinuxSyscallsTableAMD64[Frame->ax];

	long (*call)(SysFrm *, long, ...) = reinterpret_cast<long (*)(SyscallsFrame *, long, ...)>(Syscall.Handler);

	if (unlikely(!call))
	{
		fixme("Syscall %s(%d) not implemented",
			  Syscall.Name, Frame->ax);
		return -linux_ENOSYS;
	}

	debug("> [%d:\"%s\"]( %#lx  %#lx  %#lx  %#lx  %#lx  %#lx )",
		  Frame->ax, Syscall.Name,
		  Frame->di, Frame->si, Frame->dx,
		  Frame->r10, Frame->r8, Frame->r9);

	long sc_ret = call(Frame,
					   Frame->di, Frame->si, Frame->dx,
					   Frame->r10, Frame->r8, Frame->r9);

	debug("< [%ld:\"%s\"] = %ld", Frame->ax, Syscall.Name, sc_ret);
	return sc_ret;
#elif defined(__i386__)
	if (Frame->ax > sizeof(LinuxSyscallsTableI386) / sizeof(SyscallData))
	{
		fixme("Syscall %d not implemented",
			  Frame->ax);
		return -linux_ENOSYS;
	}

	SyscallData Syscall = LinuxSyscallsTableI386[Frame->ax];

	long (*call)(SysFrm *, long, ...) = reinterpret_cast<long (*)(SyscallsFrame *, long, ...)>(Syscall.Handler);

	if (unlikely(!call))
	{
		fixme("Syscall %s(%d) not implemented",
			  Syscall.Name, Frame->ax);
		return -linux_ENOSYS;
	}

	debug("> [%d:\"%s\"]( %#lx  %#lx  %#lx  %#lx  %#lx  %#lx )",
		  Frame->ax, Syscall.Name,
		  Frame->bx, Frame->cx, Frame->dx,
		  Frame->si, Frame->di, Frame->bp);

	int sc_ret = call(Frame,
					  Frame->bx, Frame->cx, Frame->dx,
					  Frame->si, Frame->di, Frame->bp);

	debug("< [%d:\"%s\"] = %d", Frame->ax, Syscall.Name, sc_ret);
	return sc_ret;
#else
	UNUSED(LinuxSyscallsTableAMD64);
	UNUSED(LinuxSyscallsTableI386);
	return -linux_ENOSYS;
#endif

#if defined(__amd64__)
	UNUSED(LinuxSyscallsTableI386);
#elif defined(__i386__)
	UNUSED(LinuxSyscallsTableAMD64);
#endif
}
