#define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE
#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <assert.h>

#define AT_NULL 0
#define AT_IGNORE 1
#define AT_EXECFD 2
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_FLAGS 8
#define AT_ENTRY 9
#define AT_NOTELF 10
#define AT_UID 11
#define AT_EUID 12
#define AT_GID 13
#define AT_EGID 14
#define AT_PLATFORM 15
#define AT_HWCAP 16
#define AT_CLKTCK 17
#define AT_SECURE 23
#define AT_BASE_PLATFORM 24
#define AT_RANDOM 25
#define AT_HWCAP2 26
#define AT_EXECFN 31
#define AT_SYSINFO 32
#define AT_SYSINFO_EHDR 33
#define AT_L1I_CACHESHAPE 34
#define AT_L1D_CACHESHAPE 35
#define AT_L2_CACHESHAPE 36
#define AT_L3_CACHESHAPE 37
#define AT_L1I_CACHESIZE 40
#define AT_L1I_CACHEGEOMETRY 41
#define AT_L1D_CACHESIZE 42
#define AT_L1D_CACHEGEOMETRY 43
#define AT_L2_CACHESIZE 44
#define AT_L2_CACHEGEOMETRY 45
#define AT_L3_CACHESIZE 46
#define AT_L3_CACHEGEOMETRY 47
#define AT_MINSIGSTKSZ 51

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct
{
	uint32_t a_type;
	union
	{
		uint32_t a_val;
	} a_un;
} Elf32_auxv_t;

typedef struct
{
	uint64_t a_type;
	union
	{
		uint64_t a_val;
	} a_un;
} Elf64_auxv_t;

#ifdef __LP64__
#define Elf_auxv_t Elf64_auxv_t
#else
#define Elf_auxv_t Elf32_auxv_t
#endif

#define loop  \
	while (1) \
		;

#define __musl_SYS_brk 12

static __inline long __musl_syscall1(long n, long a1)
{
	unsigned long ret;
#if defined(__amd64__)
	__asm__ __volatile__("syscall"
						 : "=a"(ret)
						 : "a"(n), "D"(a1)
						 : "rcx", "r11", "memory");
#endif
	return ret;
}

long __musl_syscall_ret(unsigned long r)
{
	if (r > -4096UL)
	{
		errno = -r;
		return -1;
	}
	return r;
}

void test_unaligned()
{
	printf("- Testing unaligned access\n");

	int *ptr = (int *)malloc(sizeof(int));
	uintptr_t unaligned_addr = (uintptr_t)ptr + 1;
	*(int *)unaligned_addr = 42;
	printf("%p->%p: %d\n", (void *)ptr,
		   (void *)unaligned_addr,
		   *(int *)unaligned_addr);
	free(ptr);
}

struct passwd *p = NULL;
void test_passwd()
{
	printf("- Testing passwd\n");

	p = getpwuid(getuid());
	if (p == NULL)
	{
		perror("getpwuid");
		fflush(stdout);
		fflush(stderr);
	}
	else
	{
		printf("name: %s\n", p->pw_name);
		// printf("passwd: %s\n", p->pw_passwd);
		printf("uid: %d\n", p->pw_uid);
		printf("gid: %d\n", p->pw_gid);
		// printf("gecos: %s\n", p->pw_gecos);
		printf("dir: %s\n", p->pw_dir);
		printf("shell: %s\n", p->pw_shell);
		fflush(stdout);
	}
}

static void *heap_end = NULL;
void *__libc_sbrk(intptr_t increment)
{
	if (heap_end == NULL)
	{
		heap_end = (void *)__musl_syscall1(__musl_SYS_brk, 0);
		printf("SYS_brk(0) = %p\n", heap_end);

		heap_end = (void *)__musl_syscall1(__musl_SYS_brk, (long)heap_end + 0x2000);
		printf("Adding 0x2000 to heap_end: %p\n", heap_end);
	}

	if (increment == 0)
	{
		printf("Returning heap_end: %p\n", heap_end);
		return heap_end;
	}

	void *old_heap_end = heap_end;

	long ret = __musl_syscall1(__musl_SYS_brk, (long)heap_end + increment);
	printf("SYS_brk(%p) = %p\n", (void *)((long)heap_end + increment), (void *)ret);
	if (ret == 0)
	{
		heap_end = (void *)__musl_syscall1(__musl_SYS_brk, 0);
		printf("SYS_brk(0) = %p\n", heap_end);
		return old_heap_end;
	}

	return (void *)__musl_syscall_ret(ret);
}

void test_brk()
{
	printf("- Testing brk\n");

	void *current_brk = __libc_sbrk(0);
	printf("Initial break address: %p\n", current_brk);

	void *new_brk = __libc_sbrk(0x1000);
	if (new_brk == (void *)-1)
	{
		perror("Error extending heap");
		fflush(stdout);
		fflush(stderr);
		return;
	}
	printf("Heap extended successfully.\n");

	printf("New break address: %p\n", new_brk);

	if (__libc_sbrk(current_brk - new_brk) == (void *)-1)
	{
		perror("Error reducing heap");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("Heap reduced successfully.\n");
	printf("Current break address after reduction: %p\n", __libc_sbrk(0));

	void *extended_brk = __libc_sbrk(0x1000);
	if (extended_brk == (void *)-1)
	{
		perror("Error extending heap after reduction");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("Heap extended successfully after reduction.\n");
	printf("New break address after extension: %p\n", extended_brk);

	__libc_sbrk(-0x1000);
}

void test_time()
{
	printf("- Testing time\n");

	struct tm time = {0};
	int Year = 2012;
	int Month = 6;
	int Day = 9;
	int Hour = 4;
	int Minute = 2;
	int Second = 0;

	time.tm_year = Year - 1900;
	time.tm_mon = Month - 1;
	time.tm_mday = Day;
	time.tm_hour = Hour;
	time.tm_min = Minute;
	time.tm_sec = Second;

	if (time.tm_year < 0)
		time.tm_year = 0;

	time_t t = mktime(&time);
	if (t != (time_t)-1)
	{
#ifndef __fennix__
		stime(&t);
#endif
	}
	else
	{
		perror("mktime");
		fflush(stdout);
		fflush(stderr);
	}

	time_t now = t;
	struct tm t1;
	gmtime_r(&now, &t1);
	printf("UTC: %d-%02d-%02d %02d:%02d:%02d\n", t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);
	struct tm t2;
	localtime_r(&now, &t2);
	printf("Local: %d-%02d-%02d %02d:%02d:%02d\n", t2.tm_year + 1900, t2.tm_mon + 1, t2.tm_mday, t2.tm_hour, t2.tm_min, t2.tm_sec);
	struct tm t3 = t2;
	time_t t_val = mktime(&t3);
	printf("mktime: %ld\n", (long)t_val);

	tzset();
	printf("tzname[0]: %s\n", tzname[0]);
	printf("tzname[1]: %s\n", tzname[1]);
	printf("timezone: %ld\n", timezone);
	printf("daylight: %d\n", daylight);
	fflush(stdout);
}

void test_args(int argc, char *argv[], char *envp[])
{
	printf("- Testing args\n");

	printf("%p %p %p\n",
		   (void *)(uintptr_t)&argc,
		   (void *)&argv,
		   (void *)&envp);

	printf("I have %d arguments\n", argc);
	for (int i = 0; i < argc; i++)
		printf("argv[%d] = (%p) %s\n", i, argv[i], argv[i]);

	int envc = 0;
	while (envp[envc] != NULL)
		envc++;

	printf("I have %d environment variables\n", envc);
	for (int i = 0; i < envc; i++)
		printf("envp[%d] = (%p) %s\n", i, envp[i], envp[i]);

	Elf64_auxv_t *auxv;
	char **e = envp;

	while (*e++ != NULL)
		;

	for (auxv = (Elf64_auxv_t *)e; auxv->a_type != AT_NULL; auxv++)
		printf("auxv: %ld %#lx\n", auxv->a_type, auxv->a_un.a_val);
}

void test_stdio()
{
	printf("- Testing stdin\n");

	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork");
		fflush(stdout);
		fflush(stderr);
		return;
	}
	else if (pid != 0)
		return;

	printf("Type a character: ");
	char c = getchar();
	printf("You typed: %c (%#x)\n", c, c);
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	printf("Type a line: ");
	read = getline(&line, &len, stdin);
	printf("You typed: %s (%ld bytes)\n", line, read);
	free(line);
	exit(EXIT_SUCCESS);
}

void test_file()
{
	printf("- Testing file operations\n");

	FILE *test = fopen("/etc/passwd", "r");
	if (test == NULL)
	{
		perror("fopen");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("/etc/passwd contents: ");
	char ch;
	while (1)
	{
		ch = fgetc(test);
		if (ch == EOF)
		{
			printf("\n");
			break;
		}
		putchar(ch);
	}
	fclose(test);
}

void test_stat()
{
	printf("- Testing stat\n");

	struct stat st;
	if (stat("/etc/passwd", &st) == -1)
	{
		perror("stat");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("File size: %ld\n", st.st_size);
	printf("File mode: %o\n", st.st_mode);
	printf("File inode: %ld\n", st.st_ino);

	int fd = open("/etc/passwd", O_RDONLY);
	if (fd == -1)
	{
		perror("open");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	if (fstat(fd, &st) == -1)
	{
		perror("fstat");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("File size: %ld\n", st.st_size);
	printf("File mode: %o\n", st.st_mode);
	printf("File inode: %ld\n", st.st_ino);
	close(fd);

	if (lstat("/etc/passwd", &st) == -1)
	{
		perror("lstat");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("File size: %ld\n", st.st_size);
	printf("File mode: %o\n", st.st_mode);
	printf("File inode: %ld\n", st.st_ino);
}

void test_ptmx()
{
#ifndef __fennix__
	printf("- Testing PTMX\n");

	int master, slave;
	char buffer[256];

	if (openpty(&master, &slave, NULL, NULL, NULL) == -1)
	{
		perror("openpty");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	write(master, "Hello, pty!\n", 12);
	ssize_t bytesRead = read(slave, buffer, sizeof(buffer));
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		printf("Received from pty: %s", buffer);
	}

	close(master);
	close(slave);
#endif
}

void test_system()
{
	printf("- Testing system()\n");
	int ret = system("echo Hello, world!");
	printf("system() returned %d\n", ret);
}

int sigRec = 0;
void signalHandler(int signo)
{
	printf("Signal %s received\n", strsignal(signo));
	sigRec++;
}

int expect_sighup = 0;
void sighupHandler(int)
{
	if (expect_sighup == 0)
		assert(!"SIGHUP received");
	else
		printf("SIGHUP received\n");
}

void __sig_mask_test()
{
	printf("- Testing signal masking\n");
	signal(SIGHUP, sighupHandler);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	kill(getpid(), SIGHUP);
	sleep(1);

	expect_sighup = 1;
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	sleep(1);

	signal(SIGHUP, SIG_IGN);
	expect_sighup = 0;
	kill(getpid(), SIGHUP);
	sleep(1);
}

void __sig_mask_all()
{
	printf("- Testing all signal masking\n");
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void test_signal()
{
	__sig_mask_all();
	__sig_mask_test();

	printf("- Testing Signals\n");
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGHUP, &sa, NULL);

	signal(SIGUSR1, signalHandler);
	signal(SIGUSR2, signalHandler);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	printf("Sending SIGUSR1...\n");
	kill(getpid(), SIGUSR1);
	printf("Sending SIGUSR2...\n");
	kill(getpid(), SIGUSR2);
	printf("Sending SIGHUP...\n");
	kill(getpid(), SIGHUP);
	printf("Sending SIGINT...\n");
	kill(getpid(), SIGINT);
	printf("Sending SIGQUIT...\n");
	kill(getpid(), SIGQUIT);

	printf("Signals sent\n");
	while (sigRec < 2)
		sleep(1);
}

void test_execve_fork()
{
	printf("- Testing execve and fork\n");

	pid_t pid = fork();
	if (pid == 0) // Child process
	{
		pid_t pid2 = fork();
		if (pid2 == 0) // Child process
		{
			char *shebangArgs[] = {"/test.sh", NULL};
			execv(shebangArgs[0], shebangArgs);
			perror("execv");
			fflush(stdout);
			fflush(stderr);
			exit(EXIT_SUCCESS);
		}

		printf("Creating shell process\n");
		char *args[] = {"/bin/echo", "Hello World from echo!", NULL};
		execv(args[0], args);
		perror("execv");
		fflush(stdout);
		fflush(stderr);
		exit(EXIT_SUCCESS);
	}
	else if (pid > 0)
	{
		printf("Waiting for child process %d to exit\n", pid);
		int status;
		waitpid(pid, &status, 0);
		printf("status=%#x\n", status);
		int exited = WIFEXITED(status);
		if (exited)
		{
			int exitCode = WEXITSTATUS(status);
			if (exitCode != 0)
				printf("Child process exited with code: %d\n", exitCode);
		}
		else
		{
			printf("Execution failed. (exited=%d, status=%#x)\n", exited, status);
			return;
		}
	}
	else
	{
		perror("fork");
		fflush(stdout);
		fflush(stderr);
		return;
	}
}

void test_dirent()
{
	printf("- Testing dirent\n");
	DIR *dir;
	struct dirent *entry;

	dir = opendir("/etc");
	if (dir == NULL)
	{
		perror("opendir");
		fflush(stdout);
		fflush(stderr);
		return;
	}

	printf("Contents of the directory:\n");

	int i = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		printf("%s  ", entry->d_name);
		if (++i % 5 == 0)
			printf("\n");
	}

	closedir(dir);
}

char *create_file()
{
	FILE *fp;
	char *path;

	if (p == NULL)
	{
		path = malloc(24);
		sprintf(path, "/tmp/watched_file.txt");
		fp = fopen(path, "w");
	}
	else
	{
		path = malloc(strlen(p->pw_dir) + 24);
		sprintf(path, "%s/watched_file.txt", p->pw_dir);
		fp = fopen(path, "w");
	}

	if (fp == NULL)
	{
		perror("fopen");
		fflush(stdout);
		fflush(stderr);
		return path;
	}
	fclose(fp);
	return path;
}

char *create_directory()
{
	char *path;

	if (p == NULL)
	{
		path = malloc(24);
		sprintf(path, "/tmp/watched_directory");
	}
	else
	{
		path = malloc(strlen(p->pw_dir) + 24);
		sprintf(path, "%s/watched_directory", p->pw_dir);
	}

	if (mkdir(path, 0777) < 0)
	{
		perror("mkdir");
		fflush(stdout);
		fflush(stderr);
		return path;
	}
	return path;
}

#ifndef __fennix__
void test_watch_file()
{
	printf("- Testing file watching\n");
	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork");
		fflush(stdout);
		fflush(stderr);
		return;
	}
	else if (pid != 0)
		return;

	char *path = create_file();
	int fd, wd;
	char *buffer = malloc(BUF_LEN);

	fd = inotify_init();
	if (fd < 0)
	{
		perror("inotify_init");
		exit(EXIT_FAILURE);
	}

	wd = inotify_add_watch(fd, path,
						   IN_OPEN | IN_MODIFY | IN_CLOSE);
	if (wd < 0)
	{
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}

	printf("Watching for changes in file.txt...\n");

	while (1)
	{
		ssize_t len = read(fd, buffer, BUF_LEN);
		if (len < 0)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}

		struct inotify_event *event = (struct inotify_event *)buffer;
		if (event->mask & IN_MODIFY)
			printf("File modified!\n");
		else if (event->mask & IN_OPEN)
			printf("File opened!\n");
		else if (event->mask & IN_CLOSE)
			printf("File closed!\n");
		else
			printf("Unknown event!\n");
	}

	inotify_rm_watch(fd, wd);
	close(fd);
	exit(EXIT_SUCCESS);
}

void test_watch_directory()
{
	printf("- Testing directory watching\n");
	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork");
		fflush(stdout);
		fflush(stderr);
		return;
	}
	else if (pid != 0)
		return;

	char *path = create_directory();
	int fd, wd;
	char *buffer = malloc(BUF_LEN);

	fd = inotify_init();
	if (fd < 0)
	{
		perror("inotify_init");
		exit(EXIT_FAILURE);
	}

	wd = inotify_add_watch(fd, path,
						   IN_OPEN | IN_CREATE | IN_DELETE | IN_MODIFY);
	if (wd < 0)
	{
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}

	printf("Watching for changes in the directory...\n");
	while (1)
	{
		ssize_t len = read(fd, buffer, BUF_LEN);
		if (len < 0)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}

		struct inotify_event *event = (struct inotify_event *)buffer;
		if (event->mask & IN_CREATE)
			printf("File created: %s\n", event->name);
		else if (event->mask & IN_DELETE)
			printf("File deleted: %s\n", event->name);
		else if (event->mask & IN_MODIFY)
			printf("File modified: %s\n", event->name);
		else if (event->mask & IN_OPEN)
			printf("File opened: %s\n", event->name);
		else
			printf("Unknown event!\n");
	}

	inotify_rm_watch(fd, wd);
	close(fd);
	exit(EXIT_SUCCESS);
}

void shutdown_linux()
{
	printf("- Testing shutdown\n");
	sync();
	reboot(RB_POWER_OFF);
}

void reboot_linux()
{
	printf("- Testing reboot\n");
	sync();
	reboot(RB_AUTOBOOT);
}
#endif // __fennix__

void test_stdin()
{
	printf("- Testing stdin\n");

	while (1)
	{
		printf("Input: ");
		fflush(stdout);
		char input[256];
		fgets(input, sizeof(input), stdin);
		printf("%s", input);
	}
}

void self_fork_exec()
{
	while (1)
	{
		int pid = fork();

		// if (pid >= 10)
		// {
		// 	printf("[%d] Forked %d times, exiting...\n", getpid(), pid);
		// 	kill(getpid(), SIGTERM);
		// }

		if (pid != 0)
			continue;

		printf("[%d] Executing utest(%d)...\n", getpid(), getppid());
		char *a[] = {"/bin/utest", "loop", NULL};
		int b = execv(a[0], a);
		printf("Failed to execute utest: %d\n", b);
	}
}

void fork_bomb()
{
	while (1)
	{
		printf("[%d] Forking...\n", getpid());
		int pid = fork();
		if (pid != 0)
			printf("[%d] Forked\n", pid);
		else
			printf("[%d] Child\n", getpid());
		sleep(5);
	}
}

void fork_bomb_syscall()
{
#ifdef __x86_64__
	while (1)
	{
		printf("[%d] Forking...\n", getpid());
		unsigned long pid;
		__asm__ __volatile__("syscall" : "=a"(pid)
							 : "a"(57 /* x86_64 SYS_fork */)
							 : "rcx", "r11", "memory");
		if (pid != 0)
			printf("[%ld] Forked\n", pid);
		else
			printf("[%d] Child\n", getpid());
		sleep(5);
	}
#endif
}

volatile int __dummy = 0;
int fill_stack(void *p)
{
	if (__dummy == 1)
		return (int)(__UINTPTR_TYPE__)p;
	char buf[512];
	return fill_stack(buf);
}

void TestProcess(const char *program, char *const argv[])
{
	printf("testing: %s\n", program);
	pid_t pid = fork();
	if (pid == 0)
	{
		execvp(program, argv);
		perror("execvp");
	}
	else if (pid < 0)
		perror("fork");
}

extern int simd_main();
extern int web_main();
extern int fb_main();

int main(int argc, char *argv[], char *envp[])
{
	if (argv[1] != NULL)
	{
		if (strcmp(argv[1], "loop") == 0)
		{
			printf("[%d] Looping...\n", getpid());
			while (1)
				;
		}

		if (strcmp(argv[1], "--kernel") == 0)
		{
			printf("- Starting full tests...\n");
			TestProcess("/bin/libc_test", NULL);
			printf("- test: simd\n");
			simd_main();
			printf("- test: web\n");
			web_main();
			// while (1)
			// 	;
			printf("- test: framebuffer\n");
			fb_main();
		}
	}

	printf("- Testing userspace...\n");

	// printf("Press RETURN to start tests...\n");
	// fflush(stdout);
	// char key = 0;
	// while (key != '\n' && key != '\r')
	// 	key = getchar();

	// self_fork_exec();
	// fork_bomb();
	// fork_bomb_syscall();

	// fill_stack();
	// test_stdin();
	// test_stdio();
	test_unaligned();
	test_passwd();
#ifndef __fennix__
	test_brk();
#endif
	test_time();
	test_signal();
	test_ptmx();
	test_args(argc, argv, envp);
	test_system();
	test_file();
	test_stat();
	test_dirent();
	test_execve_fork();
#ifndef __fennix__
	test_watch_file();
	test_watch_directory();
	shutdown_linux();
	reboot_linux();
#endif

	int status = 0;
	pid_t wpid;

	printf("Waiting for child processes to exit...\n");
	while ((wpid = wait(&status)) > 0)
		sleep(2);

	sync();
	printf("Userspace tests complete!\n");

	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork");
		fflush(stdout);
		fflush(stderr);
		return 1;
	}
	else if (pid == 0)
	{
		printf("Starting utest_linux...\n");
		char *args[] = {"/bin/utest_linux", NULL};
		int ret = execv(args[0], args);
		perror("execv");
		fflush(stdout);
		fflush(stderr);
		return ret;
	}

	waitpid(pid, &status, 0);

	// check if exited normally, or crashed
	if (WIFSIGNALED(status))
	{
		int signal = WTERMSIG(status);
		printf("utest_linux exited with signal: %s\n", strsignal(signal));
		return signal;
	}
	else if (WIFEXITED(status))
	{
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0)
		{
			printf("utest_linux exited with code: %d\n", exitCode);
			return exitCode;
		}
	}
	else
	{
		printf("utest_linux exited abnormally\n");
		return 1;
	}
	return 0;
}
