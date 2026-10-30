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

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

export void endpwent(void)
{
	/* FIXME */
}

export struct passwd *getpwent(void)
{
	static FILE *pwfile = NULL;
	static struct passwd pwd;
	static char line[256];

	if (pwfile == NULL)
	{
		pwfile = fopen("/etc/passwd", "r"); /* FIXME: this path should be OS dependent */
		if (pwfile == NULL)
			return NULL;
	}

	if (fgets(line, sizeof(line), pwfile) == NULL)
	{
		fclose(pwfile);
		pwfile = NULL;
		return NULL;
	}

	char *token = strtok(line, ":");
	pwd.pw_name = token;

	token = strtok(NULL, ":");
	pwd.pw_passwd = token;

	token = strtok(NULL, ":");
	pwd.pw_uid = atoi(token);

	token = strtok(NULL, ":");
	pwd.pw_gid = atoi(token);

	token = strtok(NULL, ":");
	pwd.pw_gecos = token;

	token = strtok(NULL, ":");
	pwd.pw_dir = token;

	token = strtok(NULL, ":");
	pwd.pw_shell = token;

	return &pwd;
}

export struct passwd *getpwnam(const char *name)
{
	struct passwd *pwd;
	setpwent();
	while ((pwd = getpwent()) != NULL)
	{
		if (strcmp(pwd->pw_name, name) == 0)
		{
			endpwent();
			return pwd;
		}
	}
	endpwent();
	return NULL;
}

export int getpwnam_r(const char *name, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
	FILE *pwfile;
	char line[256];
	struct passwd *pwd_entry;

	setpwent();
	while ((pwd_entry = getpwent()) != NULL)
	{
		if (strcmp(pwd_entry->pw_name, name) == 0)
		{
			*pwd = *pwd_entry;
			*result = pwd;
			endpwent();
			return 0;
		}
	}
	endpwent();
	*result = NULL;
	return 0;
}

export struct passwd *getpwuid(uid_t uid)
{
	struct passwd *pwd;
	setpwent();
	while ((pwd = getpwent()) != NULL)
	{
		if (pwd->pw_uid == uid)
		{
			endpwent();
			return pwd;
		}
	}
	endpwent();
	return NULL;
}

export int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
	FILE *pwfile;
	char line[256];
	struct passwd *pwd_entry;

	setpwent();
	while ((pwd_entry = getpwent()) != NULL)
	{
		if (pwd_entry->pw_uid == uid)
		{
			*pwd = *pwd_entry;
			*result = pwd;
			endpwent();
			return 0;
		}
	}
	endpwent();
	*result = NULL;
	return 0;
}

export void setpwent(void)
{
	/* FIXME */
}
