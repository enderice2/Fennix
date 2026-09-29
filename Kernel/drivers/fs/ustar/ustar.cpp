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

#include <driver.hpp>
#include <interface/fs.h>
#include <memory.hpp>
#include <functional>
#include <debug.h>

#include "../../../../kernel.h"

using namespace vfs;

namespace Driver::UnixStandardTAR
{
	dev_t DriverID;

	enum TypeFlag
	{
		AREGTYPE = '\0',
		REGTYPE = '0',
		LNKTYPE = '1',
		SYMTYPE = '2',
		CHRTYPE = '3',
		BLKTYPE = '4',
		DIRTYPE = '5',
		FIFOTYPE = '6',
		CONTTYPE = '7'
	};

	enum ModeFlag
	{
		TSUID = 04000,
		TSGID = 02000,
		TSVTX = 01000,
		TUREAD = 00400,
		TUWRITE = 00200,
		TUEXEC = 00100,
		TGREAD = 00040,
		TGWRITE = 00020,
		TGEXEC = 00010,
		TOREAD = 00004,
		TOWRITE = 00002,
		TOEXEC = 00001,
	};

	struct TarHeader
	{
		char name[100];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char typeflag[1];
		char link[100];
		char signature[6];
		char version[2];
		char owner[32];
		char group[32];
		char dev_maj[8];
		char dev_min[8];
		char prefix[155];
		char pad[12];
	} __packed;

	constexpr static int INODE_CHECKSUM = 0x7757A4;

#define TAR_BLOCK_SIZE 512
#define TMAGIC "ustar"
#define TMAGLEN 6
#define TVERSION "00"
#define TVERSLEN 2

	inline uint32_t GetSize(const char *String)
	{
		uint32_t ret = 0;
		while (*String)
		{
			ret *= 8;
			ret += *String - '0';
			String++;
		}
		return ret;
	}

	inline int StringToInt(const char *String)
	{
		int ret = 0;
		for (int i = 0; String[i] != '\0'; ++i)
			ret = ret * 10 + String[i] - '0';
		return ret;
	}

	class USTARInstance
	{
	public:
		struct USTARInode
		{
			Inode Node;
			off_t HeaderOffset; // Offset of the header in the device
			USTARInode *Parent;
			std::string Name;
			std::string Path;
			std::vector<USTARInode *> Children;
			bool Deleted;
			int Checksum;
		};
		std::unordered_map<ino_t, USTARInode *> Files;
		ino_t NextInode = 0;
		FileSystemDevice Device;

	private:
		ssize_t DeviceRead(void *Buffer, size_t Size, off_t Offset)
		{
			if (Device.inode.node)
				return Device.inode.ops->Read(Device.inode.node, Buffer, Size, Offset);
			else if (Device.Block)
				return Device.Block->Ops->Read(nullptr, Buffer, Size, Offset);
			else
				return -EINVAL;
		}

		ssize_t DeviceWrite(const void *Buffer, size_t Size, off_t Offset)
		{
			if (Device.inode.node)
				return Device.inode.ops->Write(Device.inode.node, Buffer, Size, Offset);
			else if (Device.Block)
				return Device.Block->Ops->Write(nullptr, Buffer, Size, Offset);
			else
				return -EINVAL;
		}

	public:
		int Lookup(Inode *_Parent, const char *Name, Inode **Result)
		{
			auto Parent = (USTARInode *)_Parent;
			debug("looking up for %s", Name);
			const char *basename;
			size_t length;
			cwk_path_get_basename(Name, &basename, &length);
			if (basename == NULL)
			{
				if (strcmp(Name, "/") == 0)
				{
					auto &it = Files.at(0);
					*Result = &it->Node;
					return 0;
				}

				error("Invalid name %s", Name);
				return -EINVAL;
			}

			if (_Parent)
			{
				for (const auto &child : Parent->Children)
				{
					if (child->Deleted || strcmp(child->Name.c_str(), basename) != 0)
						continue;

					*Result = &child->Node;
					return 0;
				}

				return -ENOENT;
			}

			auto fileItr = Files.begin();
			while (fileItr != Files.end())
			{
				USTARInode *node = fileItr->second;

				if (node->Deleted || strcmp(node->Name.c_str(), basename) != 0)
				{
					fileItr++;
					continue;
				}

				*Result = &fileItr->second->Node;
				return 0;
			}
			return -ENOENT;
		}

		int Create(Inode *_Parent, const char *Name, mode_t Mode, Inode **Result)
		{
			USTARInode *Parent = (USTARInode *)_Parent;

			Inode inode{};
			inode.Mode = Mode;
			inode.Device = DriverID;
			inode.RawDevice = 0;
			inode.Index = NextInode;
			inode.Offset = 0;
			inode.PrivateData = this;

			const char *basename;
			size_t length;
			cwk_path_get_basename(Name, &basename, &length);

			auto SetMode = [&](mode_t &Mode, TarHeader *header)
			{
				if (Mode & S_IFREG)
					header->typeflag[0] = REGTYPE;
				else if (Mode & S_IFLNK)
					header->typeflag[0] = SYMTYPE;
				else if (Mode & S_IFCHR)
					header->typeflag[0] = CHRTYPE;
				else if (Mode & S_IFBLK)
					header->typeflag[0] = BLKTYPE;
				else if (Mode & S_IFDIR)
					header->typeflag[0] = DIRTYPE;
				else if (Mode & S_IFIFO)
					header->typeflag[0] = FIFOTYPE;

				mode_t final = 0;
				if (Mode & S_ISUID)
					final |= TSUID;
				else if (Mode & S_ISGID)
					final |= TSGID;
				else if (Mode & S_ISVTX)
					final |= TSVTX;
				else if (Mode & S_IRUSR)
					final |= TUREAD;
				else if (Mode & TUWRITE)
					final |= TUWRITE;
				else if (Mode & TUEXEC)
					final |= TUEXEC;
				else if (Mode & TGREAD)
					final |= TGREAD;
				else if (Mode & TGWRITE)
					final |= TGWRITE;
				else if (Mode & TGEXEC)
					final |= TGEXEC;
				else if (Mode & TOREAD)
					final |= TOREAD;
				else if (Mode & TOWRITE)
					final |= TOWRITE;
				else if (Mode & TOEXEC)
					final |= TOEXEC;

				snprintf(header->mode, sizeof(header->mode), "%07o", final);
			};

			TarHeader *hdr = new TarHeader{};
			SetMode(inode.Mode, hdr);
			strncpy(hdr->name, basename, sizeof(hdr->name));
			strncpy(hdr->signature, TMAGIC, TMAGLEN);
			strncpy(hdr->version, TVERSION, TVERSLEN);

			USTARInode *node = new USTARInode{.Node = inode,
											  .HeaderOffset = 0,
											  .Parent = Parent,
											  .Name{},
											  .Path{},
											  .Children{},
											  .Deleted = false,
											  .Checksum = INODE_CHECKSUM};

			node->Name.assign(basename, length);
			node->Path.assign(Name, strlen(Name));

			auto file = Files.insert(std::make_pair(NextInode, node));
			assert(file.second == true);
			*Result = &Files.at(NextInode)->Node;
			if (Parent)
			{
				Parent->Children.push_back(Files.at(NextInode));
				Files.at(NextInode)->Parent = Parent;
			}
			NextInode++;
			return 0;
		}

		int Remove(Inode *Parent, const char *Name)
		{
			assert(!"not implemented");
		}

		int Rename(Inode *Parent, const char *OldName, const char *NewName)
		{
			assert(!"not implemented");
		}

		ssize_t Read(Inode *Node, void *Buffer, size_t Size, off_t Offset)
		{
			auto fileItr = Files.find(Node->Index);
			assert(fileItr != Files.end());

			if (fileItr->second->Deleted)
				return -ENOENT;

			USTARInode *node = fileItr->second;
			TarHeader header;
			if (DeviceRead(&header, sizeof(TarHeader), node->HeaderOffset) != sizeof(TarHeader))
				return -EIO;
			size_t fileSize = GetSize(header.size);

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
				Size = fileSize - Offset;
			}

			off_t dataOffset = node->HeaderOffset + sizeof(TarHeader) + Offset;
			if (DeviceRead(Buffer, Size, dataOffset) != (ssize_t)Size)
				return -EIO;
			return Size;
		}

		ssize_t Write(Inode *Node, const void *Buffer, size_t Size, off_t Offset)
		{
			assert(!"not implemented");
		}

		int Truncate(Inode *Node, off_t Size)
		{
			assert(!"not implemented");
		}

		int Open(Inode *Node, int Flags, mode_t Mode)
		{
			return 0;
		}

		int Close(Inode *Node)
		{
			return 0;
		}

		int Ioctl(Inode *Node, unsigned long Request, void *Argp)
		{
			assert(!"not implemented");
		}

		__no_sanitize("alignment") ssize_t ReadDir(Inode *_Node, kdirent *Buffer, size_t Size, off_t Offset, off_t Entries)
		{
			/* FIXME: FIX ALIGNMENT FOR DIRENT! */
			auto Node = (USTARInode *)_Node;
			debug("reading directory %s", Node->Path.c_str());

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
				debug(".");
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
				debug("..");
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

				if (var->Deleted)
					continue;

				TarHeader header;
				ssize_t hdrRead = DeviceRead(&header, sizeof(TarHeader), var->HeaderOffset);
				if (hdrRead != sizeof(TarHeader))
				{
					warn("Failed to read header for %s at offset %ld", var->Name.c_str(), var->HeaderOffset);
					continue;
				}
				debug("Entry: %s, typeflag: %c, size: %zu, offset: %ld", var->Name.c_str(), header.typeflag[0], GetSize(header.size), var->HeaderOffset);

				reclen = (uint16_t)(offsetof(struct kdirent, d_name) + var->Name.size() + 1);

				if (totalSize + reclen > Size)
				{
					debug("not enough space for %s (%zu + %zu = %zu > %zu)", var->Name.c_str(), totalSize, reclen, totalSize + reclen, Size);
					break;
				}

				ent = (struct kdirent *)((uintptr_t)Buffer + totalSize);
				ent->d_ino = var->Node.Index;
				ent->d_off = var->Node.Offset;
				ent->d_reclen = reclen;
				switch (header.typeflag[0])
				{
				case AREGTYPE:
				case REGTYPE:
					ent->d_type = DT_REG;
					break;
				case LNKTYPE:
					fixme("Hard link not implemented for %s", header.name);
					ent->d_type = DT_LNK;
					break;
				case SYMTYPE:
					ent->d_type = DT_LNK;
					break;
				case CHRTYPE:
					ent->d_type = DT_CHR;
					break;
				case BLKTYPE:
					ent->d_type = DT_BLK;
					break;
				case DIRTYPE:
					ent->d_type = DT_DIR;
					break;
				case FIFOTYPE:
					ent->d_type = DT_FIFO;
					break;
				case CONTTYPE:
				default:
					ent->d_type = 0;
					break;
				}
				strncpy(ent->d_name, var->Name.c_str(), strlen(var->Name.c_str()));
				debug("Added entry: %s, type: %d, size: %zu", var->Name.c_str(), ent->d_type, GetSize(header.size));
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

		int MkDir(Inode *Parent, const char *Name, mode_t Mode, Inode **Result)
		{
			assert(!"not implemented");
		}

		int RmDir(Inode *Parent, const char *Name)
		{
			assert(!"not implemented");
		}

		int SymLink(Inode *Parent, const char *Name, const char *Target, Inode **Result)
		{
			int ret = this->Create(Parent, Name, S_IFLNK, Result);
			if (ret < 0)
				return ret;

			USTARInode *node = (USTARInode *)*Result;
			TarHeader header;
			if (DeviceRead(&header, sizeof(TarHeader), node->HeaderOffset) != sizeof(TarHeader))
				return -EIO;
			strncpy(header.link, Target, MIN(sizeof(header.link) - 1, strlen(Target)));
			return 0;
		}

		ssize_t ReadLink(Inode *Node, char *Buffer, size_t Size)
		{
			auto fileItr = Files.find(Node->Index);
			assert(fileItr != Files.end());

			if (fileItr->second->Deleted)
				return -ENOENT;

			USTARInode *node = fileItr->second;
			TarHeader header;
			if (DeviceRead(&header, sizeof(TarHeader), node->HeaderOffset) != sizeof(TarHeader))
				return -EIO;

			size_t linkLen = 0;
			while (linkLen < sizeof(header.link) && header.link[linkLen] != '\0')
				++linkLen;
			if (linkLen > Size)
				linkLen = Size;
			memcpy(Buffer, header.link, linkLen);
			debug("Read %zu bytes from %d: \"%.*s\"", linkLen, Node->Index, (int)linkLen, Buffer);
			return linkLen;
		}

		off_t Seek(Inode *Node, off_t Offset)
		{
			assert(!"not implemented");
		}

		int Stat(Inode *Node, kstat *Stat)
		{
			auto fileItr = Files.find(Node->Index);
			assert(fileItr != Files.end());
			if (fileItr->second->Deleted)
				return -ENOENT;

			USTARInode *node = fileItr->second;
			TarHeader header;
			if (DeviceRead(&header, sizeof(TarHeader), node->HeaderOffset) != sizeof(TarHeader))
				return -EIO;
			size_t fileSize = GetSize(header.size);

			debug("Header: \"%.*s\"", (int)sizeof(struct TarHeader), &header);
			Stat->Device = DriverID;
			Stat->Index = Node->Index;
			Stat->HardLinks = 1;
			Stat->UserID = GetSize(header.uid);
			Stat->GroupID = GetSize(header.gid);
			Stat->RawDevice = Stat->MakeDevice(GetSize(header.dev_maj), GetSize(header.dev_min));
			Stat->Size = fileSize;
			Stat->AccessTime = GetSize(header.mtime);
			Stat->ModifyTime = GetSize(header.mtime);
			Stat->ChangeTime = GetSize(header.mtime);
			Stat->BlockSize = 512;
			Stat->Blocks = (fileSize + 511) / 512;
			Stat->Attribute = 0;

			mode_t hdrMode = StringToInt(header.mode);

			if (hdrMode & TSUID)
				Stat->Mode |= S_ISUID;
			else if (hdrMode & TSGID)
				Stat->Mode |= S_ISGID;
			else if (hdrMode & TSVTX)
				Stat->Mode |= S_ISVTX;
			else if (hdrMode & TUREAD)
				Stat->Mode |= S_IRUSR;
			else if (hdrMode & TUWRITE)
				Stat->Mode |= S_IWUSR;
			else if (hdrMode & TUEXEC)
				Stat->Mode |= S_IXUSR;
			else if (hdrMode & TGREAD)
				Stat->Mode |= S_IRGRP;
			else if (hdrMode & TGWRITE)
				Stat->Mode |= S_IWGRP;
			else if (hdrMode & TGEXEC)
				Stat->Mode |= S_IXGRP;
			else if (hdrMode & TOREAD)
				Stat->Mode |= S_IROTH;
			else if (hdrMode & TOWRITE)
				Stat->Mode |= S_IWOTH;
			else if (hdrMode & TOEXEC)
				Stat->Mode |= S_IXOTH;

			switch (header.typeflag[0])
			{
			case AREGTYPE:
			case REGTYPE:
				Stat->Mode |= S_IFREG;
				break;
			case LNKTYPE:
				fixme("Hard link not implemented for %s", header.name);
				Stat->Mode |= S_IFLNK;
				break;
			case SYMTYPE:
				Stat->Mode |= S_IFLNK;
				break;
			case CHRTYPE:
				Stat->Mode |= S_IFCHR;
				break;
			case BLKTYPE:
				Stat->Mode |= S_IFBLK;
				break;
			case DIRTYPE:
				Stat->Mode |= S_IFDIR;
				break;
			case FIFOTYPE:
				Stat->Mode |= S_IFIFO;
				break;
			case CONTTYPE:
				warn("Reserved type for %s", header.name);
				__fallthrough;
			default:
				error("Unknown type: %d for %s", header.typeflag[0], header.name);
				break;
			}
			return 0;
		}

		int ScanArchiveFromDevice()
		{
			off_t offset = 0;
			std::vector<USTARInode *> tmpNodes;

			auto SetMode = [&](Inode &uNode, TarHeader *header)
			{
				mode_t hdrMode = StringToInt(header->mode);
				if (hdrMode & TSUID)
					uNode.Mode |= S_ISUID;
				else if (hdrMode & TSGID)
					uNode.Mode |= S_ISGID;
				else if (hdrMode & TSVTX)
					uNode.Mode |= S_ISVTX;
				else if (hdrMode & TUREAD)
					uNode.Mode |= S_IRUSR;
				else if (hdrMode & TUWRITE)
					uNode.Mode |= S_IWUSR;
				else if (hdrMode & TUEXEC)
					uNode.Mode |= S_IXUSR;
				else if (hdrMode & TGREAD)
					uNode.Mode |= S_IRGRP;
				else if (hdrMode & TGWRITE)
					uNode.Mode |= S_IWGRP;
				else if (hdrMode & TGEXEC)
					uNode.Mode |= S_IXGRP;
				else if (hdrMode & TOREAD)
					uNode.Mode |= S_IROTH;
				else if (hdrMode & TOWRITE)
					uNode.Mode |= S_IWOTH;
				else if (hdrMode & TOEXEC)
					uNode.Mode |= S_IXOTH;

				switch (header->typeflag[0])
				{
				case AREGTYPE:
				case REGTYPE:
					uNode.Mode |= S_IFREG;
					break;
				case LNKTYPE:
					uNode.Mode |= S_IFLNK;
					break;
				case SYMTYPE:
					uNode.Mode |= S_IFLNK;
					break;
				case CHRTYPE:
					uNode.Mode |= S_IFCHR;
					break;
				case BLKTYPE:
					uNode.Mode |= S_IFBLK;
					break;
				case DIRTYPE:
					uNode.Mode |= S_IFDIR;
					break;
				case FIFOTYPE:
					uNode.Mode |= S_IFIFO;
					break;
				case CONTTYPE:
					warn("Reserved type for %s", header->name);
					__fallthrough;
				default:
					error("Unknown type: %d for %s", header->typeflag[0], header->name);
					break;
				}
			};

			while (true)
			{
				TarHeader header;
				if (DeviceRead(&header, sizeof(TarHeader), offset) != sizeof(TarHeader))
					break;
				if (strncmp(header.signature, TMAGIC, TMAGLEN - 1) != 0)
					break;
				if (isempty(header.name))
					break;

				/* This removes the "." at the beginning of the file name "./foo/bar" > "/foo/bar" */
				if (header.name[0] == '.' && header.name[1] == '/')
					memmove(header.name, header.name + 1, strlen(header.name));

				if (isempty((char *)header.name))
					fixme("Ignoring empty file name \"%.*s\"", sizeof(struct TarHeader), header);

				struct Inode uNode;
				uNode.Device = DriverID;
				uNode.RawDevice = 0;
				uNode.Index = NextInode;
				SetMode(uNode, &header);
				uNode.Offset = 0;
				uNode.PrivateData = this;

				const char *basename;
				size_t length;
				cwk_path_get_basename(header.name, &basename, &length);

				USTARInode *node = new USTARInode{.Node = uNode,
												  .HeaderOffset = offset,
												  .Parent = nullptr,
												  .Name{},
												  .Path{},
												  .Children{},
												  .Deleted = false,
												  .Checksum = INODE_CHECKSUM};

				if (basename)
					node->Name.assign(basename, length);
				else
					node->Name.assign((const char *)header.name, strlen(header.name));
				node->Path.assign((const char *)header.name, strlen(header.name));

				Files.insert(std::make_pair(NextInode, node));
				tmpNodes.push_back(node);

				size_t size = GetSize(header.size);
				offset += ((size / 512) + 1) * 512;
				if (size % 512)
					offset += 512;
				NextInode++;
			}

			/* TODO: This code can be significantly optimized but good luck understanding it */
			USTARInode *parent = nullptr;
			std::vector<USTARInode *> parentStack;
			std::vector<std::string *> pathStack;
			for (auto &file : tmpNodes)
			{
				if (file->Path == "/") /* This is root / */
				{
					parentStack.push_back(file);
					pathStack.push_back(&file->Path);
					// debug("root / generated");
					continue;
				}

				/* pathStack is never empty */

				if (file->Path.back() == '/') /* This is a directory */
				{
					const char *path = file->Path.c_str();
					size_t length;
					/* This converts /one/two/path.txt to /one/two/ */
					cwk_path_get_dirname(path, &length);
					std::string dirName(path, length);

					/* Check if the directory is at the same level as the current directory */
					if (dirName == *pathStack.back())
					{
						parent = parentStack.back();
						parentStack.push_back(file);
						pathStack.push_back(&file->Path);
						parent->Children.push_back(file);
						file->Parent = parent;
						// debug("adding \"%s\" to \"%s\"", file->Path.c_str(), parent->Path.c_str());
						continue;
					}
					else
					{
						/* Check if the directory is at a higher level */
						for (size_t i = 0; i < parentStack.size(); i++)
						{
							if (dirName != *pathStack[i])
								continue;

							/* Adjust vectors */
							while (!parentStack.empty())
							{
								if (dirName == *pathStack.back())
									break;

								// debug("popping \"%s\"", pathStack.back()->c_str());
								parentStack.pop_back();
								pathStack.pop_back();
							}

							parent = parentStack.back();
							parentStack.push_back(file);
							pathStack.push_back(&file->Path);
							parent->Children.push_back(file);
							file->Parent = parent;
							// debug("adding \"%s\" to \"%s\"", file->Path.c_str(), parent->Path.c_str());
							goto foundEnd;
						}

						// This is a new directory level
						parentStack.pop_back();
						pathStack.pop_back();
						parent = parentStack.back();
						parentStack.push_back(file);
						pathStack.push_back(&file->Path);
						parent->Children.push_back(file);
						file->Parent = parent;
						// debug("adding \"%s\" to \"%s\"", file->Path.c_str(), parent->Path.c_str());
					foundEnd:
						continue;
					}
				}

				/* From here, it's a file */

				const char *path = file->Path.c_str();
				size_t length;
				/* This converts /one/two/path.txt to /one/two/ */
				cwk_path_get_dirname(path, &length);
				std::string dirName(path, length);

				/* Check if the file is at the same level as the current directory */
				if (dirName == *pathStack.back())
				{
					parent = parentStack.back();
					parent->Children.push_back(file);
					file->Parent = parent;
					// debug("adding \"%s\" to \"%s\"", file->Path.c_str(), parent->Path.c_str());
					continue;
				}

				/* Check if the file is at a higher level */
				for (size_t i = 0; i < parentStack.size(); i++)
				{
					if (dirName != *pathStack[i])
						continue;

					/* Adjust vectors */
					while (!parentStack.empty())
					{
						if (dirName == *pathStack.back())
							break;

						// debug("popping \"%s\"", pathStack.back()->c_str());
						parentStack.pop_back();
						pathStack.pop_back();
					}

					parent = parentStack.back();
					parent->Children.push_back(file);
					file->Parent = parent;
					// debug("adding \"%s\" to \"%s\"", file->Path.c_str(), parent->Path.c_str());
				}
			}

			std::function<void(USTARInode *, int, const std::string &)> ustarTree = [&](USTARInode *node, int level, const std::string &prefix)
			{
				debug("%*s\"%s\"%ld", level * 4, prefix.c_str(), node->Name.c_str(), node->Node.Offset);
				off_t offset = 2; /* 0 . | 1 .. */
				for (auto &child : node->Children)
				{
#ifdef DEBUG
					if (offset <= 2)
					{
						if (offset == 2)
							offset = 0;
						/* Pseudo directories . and .. */
						USTARInode pseudoDot{};
						pseudoDot.Node = node->Node;
						pseudoDot.Name = ".";
						pseudoDot.Node.Offset = offset++;
						USTARInode pseudoDDot{};
						pseudoDDot.Node = node->Parent ? node->Parent->Node : node->Node;
						pseudoDDot.Name = "..";
						pseudoDDot.Node.Offset = offset++;
						ustarTree(&pseudoDot, level + 1, "|-- ");
						ustarTree(&pseudoDDot, level + 1, "|-- ");
					}
#endif

					child->Node.Offset = offset++;
					ustarTree(child, level + 1, child == node->Children.back() ? "`-- " : "|-- ");
				}
			};

			ustarTree(tmpNodes[0], 0, "");
			return 0;
		}
	};

	std::vector<USTARInstance *> Namespaces;

	int USTAR_AllocateInode(FileSystemInfo *Info, Inode **Result)
	{
		assert(!"not implemented");
	}

	int USTAR_DeleteInode(FileSystemInfo *Info, Inode *Node)
	{
		assert(!"not implemented");
	}

	int USTAR_Synchronize(FileSystemInfo *Info, Inode *Node)
	{
		assert(!"not implemented");
	}

	int USTAR_Destroy(FileSystemInfo *Info)
	{
		assert(!"not implemented");
	}

	int USTAR_Probe(FileSystemDevice *Device)
	{
		func("%#lx", Device);
		uint8_t buffer[TAR_BLOCK_SIZE];
		int bytesRead = 0;

		if (Device->Block)
			bytesRead = Device->Block->Ops->Read(nullptr, buffer, TAR_BLOCK_SIZE, 0);
		else if (Device->inode.node && Device->inode.ops && Device->inode.ops->Read)
			bytesRead = Device->inode.ops->Read(Device->inode.node, buffer, TAR_BLOCK_SIZE, 0);
		else
			return -EINVAL;

		if (bytesRead != TAR_BLOCK_SIZE)
			return -EIO;

		TarHeader *hdr = (TarHeader *)buffer;

		if (strncmp(hdr->signature, TMAGIC, TMAGLEN) != 0)
		{
			/* For some reason if GRUB inflates the archive, the magic is "ustar  " */
			if (strncmp(hdr->signature, TMAGIC, TMAGLEN - 1) == 0)
				return 0;

			debug("Invalid signature!");
			return -ENODEV;
		}

		return 0;
	}

	int USTAR_Mount(FileSystemInfo *FS, Inode **Root, FileSystemDevice *Device)
	{
		USTARInstance *instance = new USTARInstance();
		instance->Device = *Device;
		if (instance->ScanArchiveFromDevice() < 0)
		{
			delete instance;
			return -EIO;
		}
		// Find root inode (should be index 0)
		auto it = instance->Files.find(0);
		if (it == instance->Files.end())
		{
			delete instance;
			return -ENOENT;
		}
		*Root = &it->second->Node;
		(*Root)->PrivateData = instance;
		Namespaces.push_back(instance);
		return 0;
	}

	int USTAR_Unmount(FileSystemInfo *FS)
	{
		assert(!"not implemented");
	}

	int USTAR_Lookup(Inode *p, const char *nm, Inode **r) { return ((USTARInstance *)p->PrivateData)->Lookup(p, nm, r); }
	int USTAR_Create(Inode *p, const char *nm, mode_t m, Inode **r) { return ((USTARInstance *)p->PrivateData)->Create(p, nm, m, r); }
	int USTAR_Remove(Inode *p, const char *nm) { return ((USTARInstance *)p->PrivateData)->Remove(p, nm); }
	int USTAR_Rename(Inode *p, const char *on, const char *nn) { return ((USTARInstance *)p->PrivateData)->Rename(p, on, nn); }
	ssize_t USTAR_Read(Inode *n, void *b, size_t s, off_t o) { return ((USTARInstance *)n->PrivateData)->Read(n, b, s, o); }
	ssize_t USTAR_Write(Inode *n, const void *b, size_t s, off_t o) { return ((USTARInstance *)n->PrivateData)->Write(n, b, s, o); }
	int USTAR_Truncate(Inode *n, off_t s) { return ((USTARInstance *)n->PrivateData)->Truncate(n, s); }
	int USTAR_Open(Inode *n, int f, mode_t m) { return ((USTARInstance *)n->PrivateData)->Open(n, f, m); }
	int USTAR_Close(Inode *n) { return ((USTARInstance *)n->PrivateData)->Close(n); }
	int USTAR_Ioctl(Inode *n, unsigned long rq, void *ap) { return ((USTARInstance *)n->PrivateData)->Ioctl(n, rq, ap); }
	ssize_t USTAR_ReadDir(Inode *n, kdirent *b, size_t s, off_t o, off_t Entries) { return ((USTARInstance *)n->PrivateData)->ReadDir(n, b, s, o, Entries); }
	int USTAR_MkDir(Inode *p, const char *nm, mode_t m, Inode **r) { return ((USTARInstance *)p->PrivateData)->MkDir(p, nm, m, r); }
	int USTAR_RmDir(Inode *p, const char *nm) { return ((USTARInstance *)p->PrivateData)->RmDir(p, nm); }
	int USTAR_SymLink(Inode *p, const char *nm, const char *t, Inode **r) { return ((USTARInstance *)p->PrivateData)->SymLink(p, nm, t, r); }
	ssize_t USTAR_ReadLink(Inode *n, char *b, size_t s) { return ((USTARInstance *)n->PrivateData)->ReadLink(n, b, s); }
	off_t USTAR_Seek(Inode *n, off_t o) { return ((USTARInstance *)n->PrivateData)->Seek(n, o); }
	int USTAR_Stat(Inode *n, kstat *st) { return ((USTARInstance *)n->PrivateData)->Stat(n, st); }

	static SuperBlockOperations ustarSuperOps = {
		.AllocateInode = USTAR_AllocateInode,
		.DeleteInode = USTAR_DeleteInode,
		.Synchronize = USTAR_Synchronize,
		.Destroy = USTAR_Destroy,
		.Probe = USTAR_Probe,
		.Mount = USTAR_Mount,
		.Unmount = USTAR_Unmount};

	static InodeOperations ustarInodeOps = {
		.Lookup = USTAR_Lookup,
		.Create = USTAR_Create,
		.Remove = USTAR_Remove,
		.Rename = USTAR_Rename,
		.Read = USTAR_Read,
		.Write = USTAR_Write,
		.Truncate = USTAR_Truncate,
		.Open = USTAR_Open,
		.Close = USTAR_Close,
		.Ioctl = USTAR_Ioctl,
		.ReadDir = USTAR_ReadDir,
		.MkDir = USTAR_MkDir,
		.RmDir = USTAR_RmDir,
		.SymLink = USTAR_SymLink,
		.ReadLink = USTAR_ReadLink,
		.Seek = USTAR_Seek,
		.Stat = USTAR_Stat};

	int Entry()
	{
		FileSystemInfo *fsi = new FileSystemInfo;
		fsi->Name = "Unix Standard TAR";
		fsi->SuperOps = ustarSuperOps;
		fsi->Ops = ustarInodeOps;
		v0::RegisterFileSystem(DriverID, fsi);
		return 0;
	}

	int Final()
	{
		for (auto &&i : Namespaces)
			delete i;
		return 0;
	}

	int Panic() { return 0; }
	int Probe() { return 0; }

	REGISTER_BUILTIN_DRIVER(ustar,
							"Unix Standard TAR Driver",
							"enderice2",
							1, 0, 0,
							Entry,
							Final,
							Panic,
							Probe);
}
