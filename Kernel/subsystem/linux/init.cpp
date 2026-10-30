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

#include <fs/ramfs.hpp>

#include "../../kernel.h"
#include <ini.h>

namespace Subsystem::Linux
{
	bool Initialized = false;

	void __CreateStubRoot()
	{
		Node root = fs->GetRoot(0);
		Node nmnt = fs->Lookup(root, "/mnt");
		assert(MountAndRootRAMFS(nmnt, "linux", 1));
		Node linux = fs->GetRoot(1);

		Node bin = fs->Create(linux, "bin", 0755);
		Node boot = fs->Create(linux, "boot", 0755);
		Node dev = fs->Create(linux, "dev", 0755);
		Node etc = fs->Create(linux, "etc", 0755);
		Node home = fs->Create(linux, "home", 0755);
		Node lib = fs->Create(linux, "lib", 0755);
		Node lib64 = fs->Create(linux, "lib64", 0755);
		Node media = fs->Create(linux, "media", 0755);
		Node mnt = fs->Create(linux, "mnt", 0755);
		Node opt = fs->Create(linux, "opt", 0755);
		Node proc = fs->Create(linux, "proc", 0755);

		UNUSED(bin);
		UNUSED(boot);
		UNUSED(dev);
		UNUSED(etc);
		UNUSED(home);
		UNUSED(lib);
		UNUSED(lib64);
		UNUSED(media);
		UNUSED(mnt);
		UNUSED(opt);
		UNUSED(proc);
	}

	void InitializeSubSystem()
	{
		if (fs->RootExists(1) == false)
		{
			Node root = fs->GetRoot(0);
			Node cfg = fs->Lookup(root, "/etc/subsystem/linux");
			if (cfg)
			{
				struct kstat st;
				fs->Stat(cfg, &st);

				std::unique_ptr<char[]> buf(new char[st.Size]);
				fs->Read(cfg, buf.get(), st.Size, 0);

				ini_t *ini = ini_load(buf.get(), NULL);
				int section = ini_find_section(ini, "rootfs", NULL);
				int pathIdx = ini_find_property(ini, section, "path", NULL);
				const char *uPath = ini_property_value(ini, section, pathIdx);
				debug("path=%s", uPath);
				ini_destroy(ini);

				eNode ret = fs->Lookup(root, uPath);
				if (ret != false)
				{
					Node tar = ret;
					FileSystemInfo *fsi = fs->Probe(tar);
					if (fsi == nullptr)
					{
						warn("Couldn't probe rootfs %s", uPath);
						__CreateStubRoot();
					}
					else
					{
						Node mnt = fs->Lookup(root, "/mnt");
						auto node = fs->Mount(mnt, "linux", fsi, tar);
					}
				}
				else
				{
					warn("Couldn't find rootfs path %s", uPath);
					__CreateStubRoot();
				}
			}
			else
			{
				warn("Couldn't open /etc/subsystem/linux");
				__CreateStubRoot();
			}
		}
	}
}
