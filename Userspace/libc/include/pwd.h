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

#ifndef _PWD_H
#define _PWD_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <sys/types.h>

	/**
	 * /etc/passwd
	 *    name:passwd:uid:gid:gecos:dir:shell
	 *
	 * Example
	 *
	 *    root:x:0:0:root:/root:/bin/sh
	 *      |  | | |  |     |      |
	 *      |  | | |  |     |      \-- pw_shell
	 *      |  | | |  |     \-- pw_dir
	 *      |  | | |  \-- pw_gecos
	 *      |  | | \-- pw_gid
	 *      |  | \-- pw_uid
	 *      |  \-- pw_passwd
	 *      \-- pw_name
	 *
	 * @ref https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/pwd.h.html
	 * @ref https://man7.org/linux/man-pages/man3/getpw.3.html
	 * @ref https://man7.org/linux/man-pages/man5/passwd.5.html
	 */
	struct passwd
	{
		char *pw_name;	 /* User's login name. */
		char *pw_passwd; /* User password. */
		uid_t pw_uid;	 /* Numerical user ID. */
		gid_t pw_gid;	 /* Numerical group ID. */
		char *pw_gecos;	 /* User information. */
		char *pw_dir;	 /* Initial working directory. */
		char *pw_shell;	 /* Program to use as shell. */
	};

	void endpwent(void);
	struct passwd *getpwent(void);
	struct passwd *getpwnam(const char *name);
	int getpwnam_r(const char *name, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result);
	struct passwd *getpwuid(uid_t uid);
	int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result);
	void setpwent(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !_PWD_H
