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

#include <fs/vfs.hpp>

#include "../kernel.h"

// #define ENABLE_LOGGING 1

#ifdef ENABLE_LOGGING
#define dbg_vfs(...) debug(__VA_ARGS__)
#else
#define dbg_vfs(...) (void)0
#endif

namespace vfs
{
	eNode Virtual::Convert(Inode *inode)
	{
		Node cache = std::make_shared<NodeCache>();
		cache->inode = inode;
		return {cache, 0};
	}

	eNode Virtual::Convert(Node &Parent, Inode *inode)
	{
		Node cache = std::make_shared<NodeCache>();
		cache->inode = inode;
		cache->fsi = Parent->fsi;
		cache->Parent = Parent;
		Parent->Children.push_back(cache);
		return {cache, 0};
	}

	std::string Virtual::NormalizePath(Node &Parent, std::string Path, bool Join)
	{
		std::string result;
		if (Join)
		{
			size_t len = Path.size() + Parent->Path.size() + 2;
			result.reserve(len);
			len = cwk_path_join(Parent->Path.c_str(), Path.c_str(), result.data(), result.capacity());
			result.resize(len);
			return result;
		}

		size_t len = Path.size() + 2;
		result.reserve(len);
		len = cwk_path_normalize(Path.c_str(), result.data(), result.capacity());
		result.resize(len);
		return result;
	}

	bool Virtual::RootExists(dev_t Index)
	{
		if (Roots.find(Index) == Roots.end())
			return false;
		return true;
	}

	eNode Virtual::GetRoot(dev_t Index)
	{
		auto it = Roots.find(Index);
		if (it == Roots.end())
			return {nullptr, ENOENT};
		return {it->second, 0};
	}

	ssize_t Virtual::GetRoot(Node Index)
	{
		for (auto it = Roots.begin(); it != Roots.end(); ++it)
		{
			if (it->second == Index)
				return it->first;
		}
		return -ENOENT;
	}

	int Virtual::AddRoot(dev_t Index, Node Root, bool Replace)
	{
		assert(Root != nullptr);

		auto it = Roots.find(Index);
		if (it == Roots.end())
		{
			Roots[Index] = Root;
			return 0;
		}

		if (Replace)
		{
			Roots[Index] = Root;
			return 0;
		}
		else
		{
			dbg_vfs("Root %ld already exists", Index);
			return EEXIST;
		}
	}

	dev_t Virtual::RegisterFileSystem(FileSystemInfo *fsi)
	{
		assert(fsi != nullptr);
		FileSystems.insert({FileSystems.size(), fsi});
		return FileSystems.size() - 1;
	}

	int Virtual::UnregisterFileSystem(dev_t Device)
	{
		auto it = FileSystems.find(Device);
		if (it == FileSystems.end())
			return -ENOENT;

		FileSystemInfo *fsi = it->second;

		/* TODO: unmount */
		fixme("Unmounting %d", Device);

		if (fsi->SuperOps.Synchronize)
			fsi->SuperOps.Synchronize(fsi, nullptr);
		if (fsi->SuperOps.Destroy)
			fsi->SuperOps.Destroy(fsi);

		FileSystems.erase(it);
		return 0;
	}

	eNode Virtual::Lookup(Node &Parent, std::string Path)
	{
		assert(Parent != nullptr);

		dbg_vfs("looking up \"%s\" in \"%s\"", Path.c_str(), Parent->Path.c_str());

		if (Path == ".")
			return {Parent, 0};
		else if (Path == "..")
			return {Parent->Parent ? Parent->Parent : Parent, 0};

		Node base = Parent;
		bool absolute = PathIsAbsolute(Path);
		if (absolute == true)
		{
			while (base->Parent)
				base = base->Parent;
		}

		dbg_vfs("base is \"%s\" and path is \"%s\"  %d", base->Path.c_str(), Path.c_str(), absolute);
		Path = this->NormalizePath(base, Path, !absolute);
		dbg_vfs("after normalizing, path is \"%s\" %d", Path.c_str(), absolute);

		struct cwk_segment segment;
		if (!cwk_path_get_first_segment(Path.c_str(), &segment))
		{
			dbg_vfs("%s no segments; %d", Path.c_str(), absolute);
			if (Path == "/")
				return {base, 0};

			assert(!"Path doesn't have any segments.");
		}

		Node node = base;
		/* We need to go to the root after NormalizePath even if Path is relative */
		if (absolute == false)
		{
			while (node->Parent)
			{
				dbg_vfs("current parent \"%s\"", node->Parent->Path.c_str());
				node = node->Parent;
				dbg_vfs("new parent \"%s\"", node->Parent ? node->Parent->Path.c_str() : "<null>");
			}
		}

		std::string currentPath = node->Path;
		if (currentPath.empty())
			currentPath = "/";

		do
		{
			std::string segmentStr(segment.begin, segment.size);
			dbg_vfs("Current segment is \"%s\"", segmentStr.c_str());

			eNode ret = node->CachedSearch(segmentStr);
			if (ret == false)
			{
				dbg_vfs("cache miss for \"%s\"", segmentStr.c_str());

				if (node->fsi->Ops.Lookup == nullptr)
					return {nullptr, ENOTSUP};

				Inode *inode;
				int ret = node->fsi->Ops.Lookup(node->inode, segmentStr.c_str(), &inode);
				if (ret != 0)
					return {nullptr, ret};

				if (currentPath == "/")
					currentPath += segmentStr;
				else
					currentPath += "/" + segmentStr;

				node = Convert(node, inode);
				node->Name = segmentStr;
				node->Path = currentPath;
			}
			else
			{
				dbg_vfs("cache hit for \"%s\"", segmentStr.c_str());
				node = ret;
				if (currentPath == "/")
					currentPath += segmentStr;
				else
					currentPath += "/" + segmentStr;
			}
		} while (cwk_path_get_next_segment(&segment));

		return {node, 0};
	}

	eNode Virtual::Create(Node &Parent, std::string Name, mode_t Mode, bool ErrorIfExists)
	{
		eNode exists = this->Lookup(Parent, Name);
		if (exists)
		{
			if (ErrorIfExists)
				return {nullptr, EEXIST};

			/* I should handle this in a better way */
			assert((exists.Value->inode->Mode & S_IFMT) == (Mode & S_IFMT));
			dbg_vfs("File \"%s\" already exists in cache", Name.c_str());
			return exists;
		}

		if (!Parent)
			return {nullptr, EINVAL};
		if (Parent->fsi->Ops.Create == nullptr)
			return {nullptr, ENOTSUP};

		Inode *inode;
		int ret = Parent->fsi->Ops.Create(Parent->inode, Name.c_str(), Mode, &inode);
		if (ret != 0)
			return {nullptr, ret};

		Node node = Convert(Parent, inode);
		node->Name = Name;
		std::string unormalized = Parent->Path == "/" ? "/" + Name : Parent->Path + "/" + Name;
		node->Path = fs->NormalizePath(Parent, unormalized);
		return {node, 0};
	}

	int Virtual::Remove(Node &Parent, std::string Name)
	{
		if (!Parent)
			return -EINVAL;
		if (Parent->fsi->Ops.Remove == nullptr)
			return -ENOTSUP;
		int ret = Parent->fsi->Ops.Remove(Parent->inode, Name.c_str());
		if (ret == 0)
		{
			for (auto it = Parent->Children.begin(); it != Parent->Children.end(); ++it)
			{
				if (it->get()->Name != Name)
					continue;
				Parent->Children.erase(it);
				break;
			}
		}
		return ret;
	}

	int Virtual::Remove(Node &node)
	{
		if (!node->Parent)
			return -EINVAL;
		if (node->Parent->fsi->Ops.Remove == nullptr)
			return -ENOTSUP;
		int ret = node->Parent->fsi->Ops.Remove(node->inode, node->Name.c_str());
		if (ret == 0)
		{
			Node &p = node->Parent;
			for (auto it = p->Children.begin(); it != p->Children.end(); ++it)
			{
				if (it->get() != node.get())
					continue;
				p->Children.erase(it);
				break;
			}
		}
		return ret;
	}

	int Virtual::Rename(Node &node, std::string NewName)
	{
		if (node->fsi->Ops.Rename == nullptr)
			return -ENOTSUP;
		int ret = node->fsi->Ops.Rename(node->inode, node->Name.c_str(), NewName.c_str());
		if (ret == 0)
			node->Name = NewName;
		return ret;
	}

	ssize_t Virtual::Read(Node &Target, void *Buffer, size_t Size, off_t Offset)
	{
		if (Target->IsDirectory() || Target->IsMountPoint())
			return -EISDIR;

		if (Target->IsSymbolicLink())
			return -EINVAL;

		/* TODO: cache buffer */

		return Target->__Read(Buffer, Size, Offset);
	}

	ssize_t Virtual::Write(Node &Target, const void *Buffer, size_t Size, off_t Offset)
	{
		if (Target->IsDirectory() || Target->IsMountPoint())
			return -EISDIR;

		if (Target->IsSymbolicLink())
			return -EINVAL;

		/* TODO: cache buffer */

		return Target->__Write(Buffer, Size, Offset);
	}

	int Virtual::Truncate(Node &Target, off_t Size)
	{
		if (Target->IsDirectory() || Target->IsMountPoint())
			return -EISDIR;

		if (!Target->IsRegularFile())
			return -EINVAL;

		/* TODO: cache buffer */

		return Target->__Truncate(Size);
	}

	__no_sanitize("alignment") ssize_t Virtual::ReadDirectory(Node &Target, kdirent *Buffer, size_t Size, off_t Offset, off_t Entries)
	{
		if (!Target->IsDirectory() && !Target->IsMountPoint())
			return -ENOTDIR;

		ssize_t total = 0;
		off_t entryIndex = 0;
		std::list<std::string> seen;

		uint8_t *bufPtr = reinterpret_cast<uint8_t *>(Buffer);

		if (Target->fsi && Target->fsi->Ops.ReadDir)
		{
			const size_t tempBufSize = 4096;
			std::unique_ptr<uint8_t[]> tempBuf(new uint8_t[tempBufSize]);

			off_t fsOffset = Offset;
			ssize_t read = Target->fsi->Ops.ReadDir(Target->inode, (kdirent *)tempBuf.get(), tempBufSize, fsOffset, Entries);
			if (read > 0)
			{
				ssize_t pos = 0;
				while (pos < read)
				{
					kdirent *ent = (kdirent *)(tempBuf.get() + pos);
					if (ent->d_reclen == 0)
						break;

					size_t reclen = ent->d_reclen;
					if (total + reclen > Size)
						break;

					memcpy(bufPtr, ent, reclen);
					seen.push_back(ent->d_name);
					bufPtr += reclen;
					total += reclen;
					pos += reclen;
					entryIndex++;
				}
			}
		}

		for (const auto &child : Target->Children)
		{
			if (std::find(seen.begin(), seen.end(), child->Name) != seen.end())
				continue;

			if (entryIndex < Offset)
			{
				entryIndex++;
				continue;
			}

			uint16_t reclen = (uint16_t)(offsetof(struct kdirent, d_name) + child->Name.size() + 1);
			if (total + reclen > (ssize_t)Size)
				break;

			kdirent *ent = (kdirent *)bufPtr;
			ent->d_ino = child->inode ? child->inode->Index : 0;
			ent->d_off = entryIndex++;
			ent->d_reclen = reclen;
			ent->d_type = child->inode ? IFTODT(child->inode->Mode) : DT_UNKNOWN;
			strcpy(ent->d_name, child->Name.c_str());

			bufPtr += reclen;
			total += reclen;
			seen.push_back(child->Name);
		}

		return total;
	}

	__no_sanitize("alignment") std::list<Node> Virtual::ReadDirectory(Node &Target)
	{
		if (!Target->IsDirectory() && !Target->IsMountPoint())
			return {};
		std::list<Node> ret;
		std::list<std::string> seen;

		if (Target->fsi && Target->fsi->Ops.ReadDir)
		{
			const size_t bufSize = 4096;
			std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
			off_t offset = 0;
			while (true)
			{
				ssize_t read = Target->fsi->Ops.ReadDir(Target->inode, (kdirent *)buf.get(), bufSize, offset, LONG_MAX);
				if (read <= 0)
					break;
				ssize_t pos = 0;
				while (pos < read)
				{
					kdirent *ent = (kdirent *)(buf.get() + pos);
					if (ent->d_reclen == 0)
						break;
					dbg_vfs("%s", ent->d_name);
					if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
					{
						pos += ent->d_reclen;
						continue;
					}

					seen.push_back(ent->d_name);

					auto it = std::find_if(Target->Children.begin(), Target->Children.end(),
										   [&](const Node &n)
										   { return n->Name == ent->d_name; });

					if (it != Target->Children.end())
						ret.push_back(*it);
					else
					{
						eNode result = Lookup(Target, ent->d_name);
						if (result.Error == 0 && result.Value)
						{
							Target->Children.push_back(result.Value);
							result.Value->Parent = Target;
							ret.push_back(result.Value);
						}
					}
					pos += ent->d_reclen;
				}
				offset += read;
			}
		}

		for (const auto &child : Target->Children)
		{
			if (std::find(seen.begin(), seen.end(), child->Name) != seen.end())
				continue;
			if (child->Name == "." || child->Name == "..")
				continue;
			ret.push_back(child);
			seen.push_back(child->Name.c_str());
		}

		return ret;
	}

	eNode Virtual::CreateLink(Node &Parent, std::string Name, std::string Target)
	{
		mode_t mode = S_IRWXU |
					  S_IRWXG |
					  S_IRWXO |
					  S_IFLNK;

		eNode enode = this->Create(Parent, Name, mode);
		if (!enode)
			return enode;
		Node node = enode;
		node->Link = Target;
		return {node, 0};
	}

	int Virtual::Stat(Node &Target, struct kstat *Stat)
	{
		/* TODO: cache */

		return Target->__Stat(Stat);
	}

	off_t Virtual::Seek(Node &Target, off_t Offset)
	{
		/* TODO: cache */

		return Target->__Seek(Offset);
	}

	int Virtual::Open(Node &Target, int Flags, mode_t Mode)
	{
		/* TODO: cache */

		return Target->__Open(Flags, Mode);
	}

	int Virtual::Close(Node &Target)
	{
		/* TODO: cache */

		return Target->__Close();
	}

	FileSystemInfo *Virtual::Probe(FileSystemDevice *Device)
	{
		for (auto &&i : FileSystems)
		{
			if (i.second->SuperOps.Probe == nullptr)
			{
				dbg_vfs("%s does not support probing", i.second->Name);
				continue;
			}

			int ret = i.second->SuperOps.Probe(Device);
			if (ret == 0)
				return i.second;
			dbg_vfs("%s returned %d", i.second->Name, ret);
		}

		dbg_vfs("No filesystems matched");
		return nullptr;
	}

	eNode Virtual::Mount(Node &Parent, Inode *inode, std::string Name, FileSystemInfo *fsi)
	{
		assert(Parent);
		assert(inode);

		Node ret = this->Convert(inode);
		ret->fsi = fsi;
		ret->Name = Name;

		std::string unormalized = Parent->Path == "/" ? "/" + Name : Parent->Path + "/" + Name;
		ret->Path = fs->NormalizePath(Parent, unormalized);
		// ret->Link =
		ret->Parent = Parent;
		Parent->Children.push_back(ret);
		return {ret, 0};
	}

	eNode Virtual::Mount(Node &Parent, std::string Name, FileSystemInfo *fsi, FileSystemDevice *Device)
	{
		Inode *inode;
		int ret = fsi->SuperOps.Mount(fsi, &inode, Device);
		if (ret != 0)
			return {nullptr, ret};

		return this->Mount(Parent, inode, Name, fsi);

		// Node node = std::make_shared<NodeCache>();
		// node->inode = nullptr; /* FIXME: ??? */
		// node->fsi = fsi;
		// node->Flags.MountPoint = true;

		// node->Name = Name;
		// node->Path = fs->NormalizePath(Parent, Parent->Path + "/" + Name);
		// node->Parent = Parent;
		// Parent->Children.push_back(node);
		// return {node, 0};
	}

	int Virtual::Umount(Node &node)
	{
		if (!node->Flags.MountPoint)
		{
			dbg_vfs("node %s is not a mountpoint", node->Path.c_str());
			return -EINVAL;
		}

		fixme("untested code");
		std::shared_ptr<NodeCache> &ptr = node;
		ptr.reset();
		return 0;
	}

	int Virtual::Umount(Node &Parent, std::string Name)
	{
		eNode node = Parent->CachedSearch(Name);
		if (!node)
		{
			dbg_vfs("mountpoint %s not found: %s", Name.c_str(), node.what());
			return -node.Error;
		}

		return this->Umount(node.Value);
	}

	void Virtual::Initialize()
	{
		debug("Initializing virtual file system...");
		Node root = this->GetRoot(0);

		/* d rwx rwx rwx */
		mode_t mode = S_IRWXU |
					  S_IRWXG |
					  S_IRWXO |
					  S_IFDIR;
		Node var = this->Create(root, "var", mode, false);
		Node log = this->Create(var, "log", mode, false);
	}

	Virtual::Virtual() {}
	Virtual::~Virtual() {}
}
