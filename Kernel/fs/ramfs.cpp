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
#include <memory.hpp>
#include <functional>
#include <debug.h>

#include "../kernel.h"

namespace vfs
{
	int RAMFS::Lookup(struct Inode *_Parent, const char *Name, struct Inode **Result)
	{
		auto Parent = (RAMFSInode *)_Parent;

		const char *basename;
		size_t length;
		cwk_path_get_basename(Name, &basename, &length);
		if (basename == NULL)
		{
			if (strcmp(Name, RootName.c_str()) == 0)
			{
				auto &it = Files.at(0);
				*Result = &it->Node;
				return 0;
			}

			error("Invalid name %s", Name);
			return -EINVAL;
		}

		if (Parent)
		{
			for (auto &&child : Parent->Children)
			{
				if (strcmp(child->Name.c_str(), basename) != 0)
					continue;

				*Result = &child->Node;
				return 0;
			}

			return -ENOENT;
		}

		for (auto &&i : Files)
		{
			RAMFSInode *node = i.second;
			if (strcmp(node->Name.c_str(), basename) != 0)
				continue;
			*Result = &i.second->Node;
			return 0;
		}

		return -ENOENT;
	}

	int RAMFS::Create(struct Inode *_Parent, const char *Name, mode_t Mode, struct Inode **Result)
	{
		RAMFSInode *Parent = (RAMFSInode *)_Parent;

		Inode inode{};
		inode.Mode = Mode;
		inode.Device = this->DeviceID;
		inode.RawDevice = 0;
		inode.Index = NextInode;
		inode.Offset = 0;
		inode.PrivateData = this;

		const char *basename;
		size_t length;
		cwk_path_get_basename(Name, &basename, &length);

		RAMFSInode *node = new RAMFSInode;
		node->Name.assign(basename, length);
		node->Mode = Mode;
		node->Node = inode;

		auto file = Files.insert(std::make_pair(NextInode, node));
		assert(file.second == true);
		*Result = &Files.at(NextInode)->Node;
		if (Parent)
			Parent->AddChild(node);
		NextInode++;
		return 0;
	}

	ssize_t RAMFS::Read(struct Inode *Node, void *Buffer, size_t Size, off_t Offset)
	{
		auto fileItr = Files.find(Node->Index);
		assert(fileItr != Files.end());

		RAMFSInode *node = fileItr->second;
		size_t fileSize = node->Stat.Size;

		if (Size <= 0)
		{
			debug("Size is less than or equal to 0");
			Size = fileSize;
		}

		if ((size_t)Offset > fileSize)
		{
			debug("Offset %d is greater than file size %d", Offset, fileSize);
			return 0;
		}

		if ((fileSize - Offset) == 0)
		{
			debug("Offset %d is equal to file size %d", Offset, fileSize);
			return 0; /* EOF */
		}

		if ((size_t)Offset + Size > fileSize)
		{
			debug("Offset %d + Size %d is greater than file size %d",
				  Offset, Size, fileSize);
			Size = fileSize;
		}

		memcpy(Buffer, node->Buffer.Data, Size);
		return Size;
	}

	ssize_t RAMFS::Write(struct Inode *Node, const void *Buffer, size_t Size, off_t Offset)
	{
		auto fileItr = Files.find(Node->Index);
		assert(fileItr != Files.end());

		RAMFSInode *node = fileItr->second;

		if (node->Buffer.IsAllocated() == false)
			node->Buffer.Allocate(node->Stat.Size);

		size_t fileSize = node->Stat.Size;

		if (Size <= 0)
		{
			debug("Size is less than or equal to 0");
			return -EINVAL;
		}

		if ((size_t)Offset > fileSize)
		{
			debug("Offset %d is greater than file size %d", Offset, fileSize);
			node->Buffer.Allocate(Offset + Size, true, true);
		}

		if ((fileSize - Offset) == 0)
		{
			debug("Offset %d is equal to file size %d", Offset, fileSize);
			node->Buffer.Allocate(Size, true, true);
		}

		if ((size_t)Offset + Size > fileSize)
		{
			debug("Offset %d + Size %d is greater than file size %d",
				  Offset, Size, fileSize);
			node->Buffer.Allocate(Offset + Size, true, true);
		}

		memcpy(static_cast<char *>(node->Buffer.Data) + Offset, Buffer, Size);
		node->Stat.Size = Size;
		return Size;
	}

	__no_sanitize("alignment")
		ssize_t RAMFS::ReadDir(struct Inode *_Node, struct kdirent *Buffer, size_t Size, off_t Offset, off_t Entries)
	{
		/* FIXME: FIX ALIGNMENT FOR DIRENT! */
		auto Node = (RAMFSInode *)_Node;

		off_t realOffset = Offset;

		size_t totalSize = 0;
		uint16_t reclen = 0;
		struct kdirent *ent = nullptr;

		if (Offset == 0)
		{
			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen(".") + 1);
			if (totalSize + reclen >= Size)
				return -EINVAL;

			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
			ent->d_ino = Node->Node.Index;
			ent->d_off = Offset++;
			ent->d_reclen = reclen;
			ent->d_type = DT_DIR;
			strcpy(ent->d_name, ".");
			totalSize += reclen;
		}

		if (Offset <= 1)
		{
			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen("..") + 1);
			if (totalSize + reclen >= Size)
			{
				if (realOffset == 1)
					return -EINVAL;
				return totalSize;
			}

			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);

			if (Node->Parent)
				ent->d_ino = Node->Parent->Node.Index;
			else
			{
				warn("Parent is null for %s", Node->Name.c_str());
				ent->d_ino = Node->Node.Index;
			}
			ent->d_off = Offset++;
			ent->d_reclen = reclen;
			ent->d_type = DT_DIR;
			strcpy(ent->d_name, "..");
			totalSize += reclen;
		}

		if (!S_ISDIR(Node->Node.Mode))
			return -ENOTDIR;

		if ((Offset >= 2 ? (Offset - 2) : Offset) > (off_t)Node->Children.size())
			return -EINVAL;

		off_t entries = 0;
		for (const auto &var : Node->Children)
		{
			if (var->Node.Offset < Offset)
				continue;

			if (entries >= Entries)
				break;

			reclen = (uint16_t)(offsetof(struct kdirent, d_name) + strlen(var->Name.c_str()) + 1);

			if (totalSize + reclen >= Size)
				break;

			ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
			ent->d_ino = var->Node.Index;
			ent->d_off = var->Node.Offset;
			ent->d_reclen = reclen;

			if (S_ISREG(var->Stat.Mode))
				ent->d_type = DT_REG;
			else if (S_ISDIR(var->Stat.Mode))
				ent->d_type = DT_DIR;
			else if (S_ISLNK(var->Stat.Mode))
				ent->d_type = DT_LNK;
			else if (S_ISCHR(var->Stat.Mode))
				ent->d_type = DT_CHR;
			else if (S_ISBLK(var->Stat.Mode))
				ent->d_type = DT_BLK;
			else if (S_ISFIFO(var->Stat.Mode))
				ent->d_type = DT_FIFO;
			else if (S_ISSOCK(var->Stat.Mode))
				ent->d_type = DT_SOCK;
			else
				ent->d_type = DT_UNKNOWN;

			strncpy(ent->d_name, var->Name.c_str(), strlen(var->Name.c_str()));
			totalSize += reclen;
			entries++;
		}

		if (totalSize + sizeof(struct kdirent) >= Size)
			return totalSize;

		ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
		ent->d_ino = 0;
		ent->d_off = 0;
		ent->d_reclen = 0;
		ent->d_type = DT_UNKNOWN;
		ent->d_name[0] = '\0';
		return totalSize;
	}

	int RAMFS::SymLink(struct Inode *Node, const char *Name, const char *Target, struct Inode **Result)
	{
		int ret = this->Create(Node, Name, S_IFLNK, Result);
		if (ret < 0)
			return ret;

		RAMFSInode *node = (RAMFSInode *)*Result;
		node->SymLink.assign(Target, strlen(Target));
		return 0;
	}

	ssize_t RAMFS::ReadLink(struct Inode *Node, char *Buffer, size_t Size)
	{
		auto fileItr = Files.find(Node->Index);
		assert(fileItr != Files.end());

		RAMFSInode *node = fileItr->second;

		if (node->SymLink.size() > Size)
			Size = node->SymLink.size();

		strncpy(Buffer, node->SymLink.data(), Size);
		debug("Read link %d bytes from %d: \"%s\"", Size, Node->Index, Buffer);
		return Size;
	}

	int RAMFS::Stat(struct Inode *Node, struct kstat *Stat)
	{
		auto fileItr = Files.find(Node->Index);
		assert(fileItr != Files.end());

		RAMFSInode *node = fileItr->second;
		*Stat = node->Stat;
		return 0;
	}
}

int __ramfs_Lookup(struct Inode *Parent, const char *Name, struct Inode **Result)
{
	return ((vfs::RAMFS *)Parent->PrivateData)->Lookup(Parent, Name, Result);
}

int __ramfs_Create(struct Inode *Parent, const char *Name, mode_t Mode, struct Inode **Result)
{
	return ((vfs::RAMFS *)Parent->PrivateData)->Create(Parent, Name, Mode, Result);
}

ssize_t __ramfs_Read(struct Inode *Node, void *Buffer, size_t Size, off_t Offset)
{
	return ((vfs::RAMFS *)Node->PrivateData)->Read(Node, Buffer, Size, Offset);
}

ssize_t __ramfs_Write(struct Inode *Node, const void *Buffer, size_t Size, off_t Offset)
{
	return ((vfs::RAMFS *)Node->PrivateData)->Write(Node, Buffer, Size, Offset);
}

ssize_t __ramfs_Readdir(struct Inode *Node, struct kdirent *Buffer, size_t Size, off_t Offset, off_t Entries)
{
	return ((vfs::RAMFS *)Node->PrivateData)->ReadDir(Node, Buffer, Size, Offset, Entries);
}

int __ramfs_SymLink(Inode *Parent, const char *Name, const char *Target, Inode **Result)
{
	return ((vfs::RAMFS *)Parent->PrivateData)->SymLink(Parent, Name, Target, Result);
}

ssize_t __ramfs_ReadLink(Inode *Node, char *Buffer, size_t Size)
{
	return ((vfs::RAMFS *)Node->PrivateData)->ReadLink(Node, Buffer, Size);
}

int __ramfs_Stat(struct Inode *Node, kstat *Stat)
{
	return ((vfs::RAMFS *)Node->PrivateData)->Stat(Node, Stat);
}

int __ramfs_DestroyInode(FileSystemInfo *Info, Inode *Node)
{
	vfs::RAMFS::RAMFSInode *inode = (vfs::RAMFS::RAMFSInode *)Node;
	delete inode;
	return 0;
}

int __ramfs_Destroy(FileSystemInfo *fsi)
{
	assert(fsi->PrivateData);
	delete (vfs::RAMFS *)fsi->PrivateData;
	delete fsi;
	return 0;
}

bool MountAndRootRAMFS(Node Parent, const char *Name, size_t Index)
{
	vfs::RAMFS *ramfs = new vfs::RAMFS;
	ramfs->RootName.assign(Name);

	FileSystemInfo *fsi = new FileSystemInfo;
	fsi->Name = "ramfs";
	fsi->SuperOps.DeleteInode = __ramfs_DestroyInode;
	fsi->SuperOps.Destroy = __ramfs_Destroy;
	fsi->Ops.Lookup = __ramfs_Lookup;
	fsi->Ops.Create = __ramfs_Create;
	fsi->Ops.Read = __ramfs_Read;
	fsi->Ops.Write = __ramfs_Write;
	fsi->Ops.ReadDir = __ramfs_Readdir;
	fsi->Ops.SymLink = __ramfs_SymLink;
	fsi->Ops.ReadLink = __ramfs_ReadLink;
	fsi->Ops.Stat = __ramfs_Stat;
	fsi->PrivateData = ramfs;
	ramfs->DeviceID = fs->RegisterFileSystem(fsi);

	Inode *root = nullptr;
	ramfs->Create(nullptr, Name, S_IFDIR | 0755, &root);

	eNode mounted = fs->Mount(Parent, root, Name, fsi);
	if (!mounted)
		return false;
	return fs->AddRoot(Index, mounted) == 0;
}
