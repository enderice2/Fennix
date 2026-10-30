/*
	This file is part of Fennix C Library.

	Fennix C Library is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix C Library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix C Library. If not, see <https://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <bits/libc.h>
#include <fcntl.h>

export char *optarg;
export int optind, opterr, optopt;
export char **environ;

export int access(const char *path, int amode)
{
	return __check_errno(sysdep(Access)(path, amode), -1);
}

export unsigned int alarm(unsigned int seconds)
{
	printf("alarm() is unimplemented\n");
	return __check_errno(-ENOSYS, -1);
}

export int brk(void *addr)
{
	return __check_errno(sysdep(Brk)(addr), -1);
}

export int chdir(const char *path)
{
	return __check_errno(sysdep(ChangeDirectory)(path), -1);
}

export int chroot(const char *);
export int chown(const char *, uid_t, gid_t);

export int close(int fildes)
{
	return __check_errno(sysdep(Close)(fildes), -1);
}

export size_t confstr(int, char *, size_t);
export char *crypt(const char *, const char *);
export char *ctermid(char *);
export char *cuserid(char *s);
export int dup(int);
export int dup2(int, int);
export void encrypt(char[64], int);

export int execl(const char *path, const char *arg0, ...)
{
	va_list args;
	va_start(args, arg0);

	int argc = 1;
	while (va_arg(args, const char *))
		argc++;
	va_end(args);

	char *argv[argc + 1];
	va_start(args, arg0);
	argv[0] = (char *)arg0;
	for (int i = 1; i < argc; i++)
		argv[i] = va_arg(args, char *);

	argv[argc] = NULL;
	va_end(args);
	return execve(path, argv, environ);
}

export int execle(const char *path, const char *arg0, ...)
{
	va_list args;
	va_start(args, arg0);

	int argc = 1;
	while (va_arg(args, const char *))
		argc++;
	va_end(args);

	char *argv[argc + 1];
	va_start(args, arg0);
	argv[0] = (char *)arg0;
	for (int i = 1; i < argc; i++)
		argv[i] = va_arg(args, char *);

	argv[argc] = NULL;

	char *const *envp = va_arg(args, char *const *);
	va_end(args);

	return execve(path, argv, envp);
}

export int execlp(const char *file, const char *arg0, ...)
{
	va_list args;
	va_start(args, arg0);

	int argc = 1;
	while (va_arg(args, const char *))
		argc++;

	va_end(args);

	char *argv[argc + 1];
	va_start(args, arg0);
	argv[0] = (char *)arg0;
	for (int i = 1; i < argc; i++)
		argv[i] = va_arg(args, char *);

	argv[argc] = NULL;
	va_end(args);

	return execvp(file, argv);
}

export int execv(const char *path, char *const argv[])
{
	return execve(path, argv, environ);
}

export int execve(const char *path, char *const argv[], char *const envp[])
{
	return __check_errno(sysdep(Execve)(path, argv, envp), -1);
}

export int execvp(const char *file, char *const argv[])
{
	if (strchr(file, '/'))
		return execve(file, argv, environ);

	char *path = getenv("PATH");
	if (!path)
	{
		errno = ENOENT;
		return -1;
	}

	char *p = strtok(path, ":");
	while (p)
	{
		char fullpath[PATH_MAX];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", p, file);
		execve(fullpath, argv, environ);
		if (errno != ENOENT && errno != ENOTDIR)
			return -1;
		p = strtok(NULL, ":");
	}

	errno = ENOENT;
	return -1;
}

export void _exit(int);
export int fchown(int, uid_t, gid_t);
export int fchdir(int);
export int fdatasync(int);

export pid_t fork(void)
{
	return __check_errno(sysdep(Fork)(), -1);
}

export long int fpathconf(int, int);
export int fsync(int);
export int ftruncate(int, off_t);

export char *getcwd(char *buf, size_t size)
{
	return (char *)__check_errno((__iptr)sysdep(GetWorkingDirectory)(buf, size), (__iptr)NULL);
}

export int getdtablesize(void);
export gid_t getegid(void);
export uid_t geteuid(void);
export gid_t getgid(void);
export int getgroups(int, gid_t[]);
export long gethostid(void);

export int gethostname(char *name, size_t namelen)
{
	if (namelen == 0)
		return -1;

	char *hostname = getenv("HOSTNAME");
	if (hostname)
	{
		strncpy(name, hostname, namelen);
		return 0;
	}

	int fd = open("/etc/hostname", O_RDONLY); /* FIXME: this path should be OS dependent */
	if (fd == -1)
		return -1;

	int result = read(fd, name, namelen);
	close(fd);
	if (result == -1)
		return -1;

	for (int i = 0; i < namelen; i++)
	{
		if (name[i] == '\n' || name[i] == '\r')
			name[i] = '\0';
		else if (name[i] < ' ' || name[i] > '~')
			name[i] = '\0';
	}

	return 0;
}

export char *getlogin(void);
export int getlogin_r(char *, size_t);
export int getopt(int, char *const[], const char *);

export int getpagesize(void) { return 0x1000; } /* TODO: getpagesize */

export char *getpass(const char *);
export pid_t getpgid(pid_t);
export pid_t getpgrp(void);

export pid_t getpid(void)
{
	return sysdep(GetProcessID)();
}

export pid_t getppid(void)
{
	return sysdep(GetParentProcessID)();
}

export pid_t getsid(pid_t);

export uid_t getuid(void)
{
	/* FIXME: getuid */
	return 0;
	// return call_getuid();
}

export char *getwd(char *);
export int isatty(int);
export int lchown(const char *, uid_t, gid_t);
export int link(const char *, const char *);
export int lockf(int, int, off_t);
export off_t lseek(int, off_t, int);
export int nice(int);
export long int pathconf(const char *, int);

export int pause(void)
{
	printf("pause() is unimplemented\n");
	return __check_errno(-ENOSYS, -1);
}

export int pipe(int[2]);

export ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
	return __check_errno(sysdep(PRead)(fildes, buf, nbyte, offset), -1);
}

export int pthread_atfork(void (*)(void), void (*)(void), void (*)(void));

export ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset)
{
	return __check_errno(sysdep(PWrite)(fildes, buf, nbyte, offset), -1);
}

export ssize_t read(int fildes, void *buf, size_t nbyte)
{
	return __check_errno(sysdep(Read)(fildes, buf, nbyte), -1);
}

export int readlink(const char *, char *, size_t);

export int rmdir(const char *path)
{
	return __check_errno(sysdep(RemoveDirectory)(path), -1);
}

export void *sbrk(intptr_t incr);
export int setgid(gid_t);
export int setpgid(pid_t, pid_t);
export pid_t setpgrp(void);
export int setregid(gid_t, gid_t);
export int setreuid(uid_t, uid_t);
export pid_t setsid(void);
export int setuid(uid_t);

export unsigned int sleep(unsigned int seconds)
{
	unsigned int unslept = alarm(0); /* Cancel any existing alarm */
	if (unslept > 0)
	{
		alarm(unslept); /* Restore the previous alarm if it was set */
		return unslept;
	}

	alarm(seconds); /* Set the alarm for the requested sleep time */
	pause();		/* Suspend execution until a signal is received */

	unslept = alarm(0); /* Cancel the alarm and get the remaining time */
	return unslept;
}

export void swab(const void *, void *, ssize_t);
export int symlink(const char *, const char *);

export void sync(void)
{
	printf("sync() is unimplemented\n");
}

export long int sysconf(int);
export pid_t tcgetpgrp(int);
export int tcsetpgrp(int, pid_t);
export int truncate(const char *, off_t);
export char *ttyname(int);
export int ttyname_r(int, char *, size_t);
export useconds_t ualarm(useconds_t, useconds_t);

export int unlink(const char *path)
{
	return __check_errno(sysdep(Unlink)(path), -1);
}

export int usleep(useconds_t);
export pid_t vfork(void);

export ssize_t write(int fildes, const void *buf, size_t nbyte)
{
	return __check_errno(sysdep(Write)(fildes, buf, nbyte), -1);
}
