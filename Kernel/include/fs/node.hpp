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

#include <interface/fs.h>
#include <errno.h>
#include <string>
#include <vector>

/**
 * This macro is used to check if a filesystem operation is available.
 *
 * TL;DR
 *
 * @code
 * if FileSystemInfo.Ops.op == nullptr
 *     return -err
 * else
 *     return FileSystemInfo.Ops.op(this->Node, ...);
 * @endcode
 *
 * @param op The operation to check.
 * @param err The error to return if the operation is not available.
 * @param ... The arguments to pass to the operation.
 *
 * @return The result of the operation.
 */
#define __check_op(op, err, ...)          \
	if (unlikely(fsi->Ops.op == nullptr)) \
		return -err;                      \
	else                                  \
		return fsi->Ops.op(this->inode, ##__VA_ARGS__)

namespace vfs
{
	class NodeObject
	{
	public:
		Inode *inode;
		FileSystemInfo *fsi;
		union
		{
			struct
			{
				uint8_t MountPoint : 1;
				uint8_t __reserved : 7;
			};
			uint8_t raw;
		} Flags;

		int __Lookup(const char *Name, Inode **Node) { __check_op(Lookup, ENOTSUP, Name, Node); }
		int __Create(const char *Name, mode_t Mode, Inode **Node) { __check_op(Create, ENOTSUP, Name, Mode, Node); }
		int __Remove(const char *Name) { __check_op(Remove, ENOTSUP, Name); }
		int __Rename(const char *OldName, const char *NewName) { __check_op(Rename, ENOTSUP, OldName, NewName); }
		ssize_t __Read(auto Buffer, size_t Size, off_t Offset) { __check_op(Read, ENOTSUP, (void *)Buffer, Size, Offset); }
		ssize_t __Write(const auto Buffer, size_t Size, off_t Offset) { __check_op(Write, ENOTSUP, (const void *)Buffer, Size, Offset); }
		int __Truncate(off_t Size) { __check_op(Truncate, ENOTSUP, Size); }
		int __Open(int Flags, mode_t Mode) { __check_op(Open, ENOTSUP, Flags, Mode); }
		int __Close() { __check_op(Close, ENOTSUP); }
		int __Ioctl(unsigned long Request, void *Argp) { __check_op(Ioctl, ENOTSUP, Request, Argp); }
		ssize_t __ReadDir(struct kdirent *Buffer, size_t Size, off_t Offset, off_t Entries) { __check_op(ReadDir, ENOTSUP, Buffer, Size, Offset, Entries); }
		int __MkDir(const char *Name, mode_t Mode, struct Inode **Result) { __check_op(MkDir, ENOTSUP, Name, Mode, Result); }
		int __RmDir(const char *Name) { __check_op(RmDir, ENOTSUP, Name); }
		int __SymLink(const char *Name, const char *Target, struct Inode **Result) { __check_op(SymLink, ENOTSUP, Name, Target, Result); }
		ssize_t __ReadLink(auto Buffer, size_t Size) { __check_op(ReadLink, ENOTSUP, (char *)Buffer, Size); }
		off_t __Seek(off_t Offset) { __check_op(Seek, ENOTSUP, Offset); }
		int __Stat(struct kstat *Stat) { __check_op(Stat, ENOTSUP, Stat); }

		~NodeObject()
		{
			debug("%#lx destructor called", this);
		}
	};
}

class NodeCache;

/**
 * @brief Node is a type that represents a filesystem node.
 * It is a shared pointer to a NodeCache object.
 *
 * If the refcount of the NodeCache object goes to zero, the data is synced to disk.
 */
typedef std::shared_ptr<NodeCache> Node;

/**
 * @brief NodeResult is a type that represents the result of a filesystem operation.
 * It contains a Node object and an error code.
 */
typedef struct NodeResult
{
	/* Value must be the first member of the struct to allow for implicit conversion */
	Node Value;
	int Error;

	operator bool() const { return Error == 0 && Value.get() != nullptr; }
	operator Node() const { return Value; }
	const char *what() const { return strerror(Error); }
} eNode;

class NodeCache : public vfs::NodeObject
{
public:
	std::string Name, Path, Link;

	/**
	 * @brief Parent of this node
	 *
	 * Maximum depth is 1, otherwise undefined behavior.
	 */
	Node Parent;

	/**
	 * @brief Childrens of this node
	 *
	 * On access, children are loaded, but not children of children!
	 * Accessing children of children is undefined behavior.
	 */
	std::vector<Node> Children;

	std::string GetName() { return Name; }
	std::string GetPath() { return Path; }
	std::string GetLink() { return Link; }

	bool IsDirectory() { return S_ISDIR(inode->Mode); }
	bool IsCharacterDevice() { return S_ISCHR(inode->Mode); }
	bool IsBlockDevice() { return S_ISBLK(inode->Mode); }
	bool IsRegularFile() { return S_ISREG(inode->Mode); }
	bool IsFIFO() { return S_ISFIFO(inode->Mode); }
	bool IsSymbolicLink() { return S_ISLNK(inode->Mode); }
	bool IsSocket() { return S_ISSOCK(inode->Mode); }
	bool IsMountPoint() { return Flags.MountPoint; }

	/**
	 * @brief Search through the cached children of this node
	 *
	 * Searches through the cached children of this node.
	 *
	 * @param Name The name to search for
	 * @return 0 and a Node on success, ENOENT on failure
	 */
	eNode CachedSearch(std::string Name)
	{
		for (auto it = Children.begin(); it != Children.end(); ++it)
		{
			if ((*it).get() == nullptr)
			{
				Children.erase(it);
				continue;
			}

			// debug("comparing \"%s\" with \"%s\"", (*it)->Name.c_str(), Name.c_str());
			if ((*it)->Name == Name)
			{
				// debug("\"%s\" found", Name.c_str());
				return {*it, 0};
			}
		}

		return {nullptr, ENOENT};
	}

	/**
	 * @brief Get the allocation size of this object
	 *
	 * @return The allocated size
	 */
	size_t GetAllocationSize()
	{
		size_t size = sizeof(NodeCache);
		size += Name.capacity();
		size += Path.capacity();
		size += Link.capacity();
		size += Children.capacity();
		return size;
	}

	~NodeCache()
	{
		debug("%#lx\"%s\" destructor called", this, Name.c_str());

		if (this->Parent)
			this->Parent->Children.erase(std::remove(this->Parent->Children.begin(), this->Parent->Children.end(), Node(this)), this->Parent->Children.end());

		if (fsi->SuperOps.Synchronize)
			fsi->SuperOps.Synchronize(this->fsi, this->inode);

		// FIXME: recursive deletion of nodes children
	}
};

#undef __check_op
