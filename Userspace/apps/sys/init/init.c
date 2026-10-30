/*
	This file is part of Fennix Userspace.

	Fennix Userspace is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	Fennix Userspace is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Fennix Userspace. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void StartProcess(const char *program, char *const argv[])
{
	pid_t pid = fork();
	if (pid == 0)
	{
		execvp(program, argv);
		perror("execvp");
		exit(EXIT_FAILURE);
	}
	else if (pid < 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
}

void ReapZombies()
{
	while (1)
	{
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid <= 0)
			break;
	}
}

void HandleSignal(int signal)
{
	if (signal == SIGTERM || signal == SIGINT)
	{
		printf("init: received termination signal, shutting down...\n");
		exit(0);
	}
}

int main()
{
	printf("init starting...\n");

	signal(SIGTERM, HandleSignal);
	signal(SIGINT, HandleSignal);

	char *shellArgs[] = {"/bin/sh", NULL};
	StartProcess("/bin/sh", shellArgs);

	// char *dummyServiceArgs[] = {"/usr/bin/dummy_service", NULL};
	// StartProcess("/usr/bin/dummy_service", dummyServiceArgs);

	while (1)
	{
		ReapZombies();
		sleep(1);
	}

	return 0;
}
