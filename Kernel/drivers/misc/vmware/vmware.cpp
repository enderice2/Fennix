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

#if defined(__amd64__) || defined(__i386__)

#include <driver.hpp>
#include <cpu.hpp>
#include <pci.hpp>
#include <io.h>

extern Driver::Manager *DriverManager;
extern PCI::Manager *PCIManager;
namespace Driver::VMwareToolBox
{
	dev_t DriverID;

	enum RPCMessages
	{
		MSG_OPEN,
		MSG_SENDSIZE,
		MSG_SENDPAYLOAD,
		MSG_RECVSIZE,
		MSG_RECVPAYLOAD,
		MSG_RECVSTATUS,
		MSG_CLOSE,
	};

	enum RPCStatus
	{
		STATUS_SUCCESS = 0x1,
		STATUS_DORECV = 0x2,
		STATUS_CPT = 0x10,
		STATUS_HB = 0x80,
	};

	typedef struct
	{
		union
		{
			uint32_t ax;
			uint32_t magic;
		};
		union
		{
			uint32_t bx;
			size_t size;
		};
		union
		{
			uint32_t cx;
			uint16_t command;
		};
		union
		{
			uint32_t dx;
			uint16_t port;
		};
		uint32_t si;
		uint32_t di;
	} VMwareCommand;

#define VMWARE_MAGIC 0x564D5868

#define VMWARE_PORT 0x5658
#define VMWARE_PORTHB 0x5659

#define VMWARE_HYPERVISOR_HB 0x00000000
#define VMWARE_HYPERVISOR_OUT 0x00000001

#define CMD_GETVERSION 0xA
#define CMD_MESSAGE 0x1E
#define CMD_ABSPOINTER_DATA 0x27
#define CMD_ABSPOINTER_STATUS 0x28
#define CMD_ABSPOINTER_COMMAND 0x29

#define ABSPOINTER_ENABLE 0x45414552
#define ABSPOINTER_RELATIVE 0xF5
#define ABSPOINTER_ABSOLUTE 0x53424152

#define MESSAGE_RPCI 0x49435052
#define MESSAGE_TCLO 0x4f4c4354

#define FLAG_COOKIE 0x80000000

#define ToMsg(x) ((x) << 16 | CMD_MESSAGE)
#define HighWord(x) ((x & 0xFFFF0000) >> 16)

#define MESSAGE_HB_MSG 0

#define MESSAGE_OPEN_CHANNEL ToMsg(MSG_OPEN)
#define MESSAGE_CLOSE_CHANNEL ToMsg(MSG_CLOSE)

#define MESSAGE_SEND_SIZE ToMsg(MSG_SENDSIZE)
#define MESSAGE_SEND_PAYLOAD ToMsg(MSG_SENDPAYLOAD)

#define MESSAGE_RECV_SIZE ToMsg(MSG_RECVSIZE)
#define MESSAGE_RECV_PAYLOAD ToMsg(MSG_RECVPAYLOAD)
#define MESSAGE_RECV_STATUS ToMsg(MSG_RECVSTATUS)

#if defined(__amd64__)

#define VM_PORT(cmd, in_ebx, isi, idi,          \
				flags, magic,                   \
				ax, bx, cx, dx, si, di)         \
	__asm__ __volatile__("movw $0x5658, %%dx\n" \
						 "inl %%dx, %%eax\n"    \
						 : "=a"(ax),            \
						   "=b"(bx),            \
						   "=c"(cx),            \
						   "=d"(dx),            \
						   "=S"(si),            \
						   "=D"(di)             \
						 : "a"(magic),          \
						   "b"(in_ebx),         \
						   "c"(cmd),            \
						   "d"(flags),          \
						   "S"(isi),            \
						   "D"(idi) : "memory")

#define VM_PORT_HB_OUT(cmd, in_ecx, isi, idi,   \
					   flags, magic, bp,        \
					   ax, bx, cx, dx, si, di)  \
	__asm__ __volatile__("push %%rbp\n"         \
						 "mov %12, %%rbp\n"     \
						 "movw $0x5659, %%dx\n" \
						 "rep outsb\n"          \
						 "pop %%rbp\n"          \
						 : "=a"(ax),            \
						   "=b"(bx),            \
						   "=c"(cx),            \
						   "=d"(dx),            \
						   "=S"(si),            \
						   "=D"(di)             \
						 : "a"(magic),          \
						   "b"(cmd),            \
						   "c"(in_ecx),         \
						   "d"(flags),          \
						   "S"(isi),            \
						   "D"(idi),            \
						   "r"(bp) : "memory", "cc")

#define VM_PORT_HB_IN(cmd, in_ecx, isi, idi,    \
					  flags, magic, bp,         \
					  ax, bx, cx, dx, si, di)   \
	__asm__ __volatile__("push %%rbp\n"         \
						 "mov %12, %%rbp\n"     \
						 "movw $0x5659, %%dx\n" \
						 "rep insb\n"           \
						 "pop %%rbp\n"          \
						 : "=a"(ax),            \
						   "=b"(bx),            \
						   "=c"(cx),            \
						   "=d"(dx),            \
						   "=S"(si),            \
						   "=D"(di)             \
						 : "a"(magic),          \
						   "b"(cmd),            \
						   "c"(in_ecx),         \
						   "d"(flags),          \
						   "S"(isi),            \
						   "D"(idi),            \
						   "r"(bp) : "memory", "cc")

#elif defined(__i386__)

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define VM_PORT(cmd, in_ebx, isi, idi, \
				flags, magic,          \
				ax, bx, cx, dx, si, di)

#define VM_PORT_HB_OUT(cmd, in_ecx, isi, idi, \
					   flags,                 \
					   magic, bp, ax,         \
					   bx, cx, dx, si, di)

#define VM_PORT_HB_IN(cmd, in_ecx, isi, idi, \
					  flags, magic, bp,      \
					  ax, bx, cx, dx, si, di)

#endif

	/* TODO:
		- use vmcall or vmmcall instead of "out" and "in" if available
	*/

	typedef struct
	{
		int TCLOChannel;
		uint16_t ChannelID;
		uint32_t CookieHigh;
		uint32_t CookieLow;
	} ToolboxContext;

	dev_t MouseDevID = -1;

	int __strcmp(const char *l, const char *r)
	{
		for (; *l == *r && *l; l++, r++)
			;

		return *(unsigned char *)l - *(unsigned char *)r;
	}

	void __cpuid(uint32_t Function,
				 uint32_t *eax, uint32_t *ebx,
				 uint32_t *ecx, uint32_t *edx)
	{
		asmv("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(Function));
	}

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD PS2_STATUS

#define PS2_CMD_ENABLE_PORT_2 0xA8
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT 0xD4
#define PS2_MOUSE_CMD_SET_DEFAULTS 0xF6
#define PS2_MOUSE_CMD_ENABLE_DATA_REPORTING 0xF4
#define PS2_MOUSE_CMD_DISABLE_DATA_REPORTING 0xF5

	typedef union
	{
		struct
		{
			uint8_t OutputBufferFull : 1;
			uint8_t InputBufferFull : 1;
			uint8_t SystemFlag : 1;
			uint8_t CommandData : 1;
			uint8_t Unknown1 : 1;
			uint8_t Unknown2 : 1;
			uint8_t TimeoutError : 1;
			uint8_t ParityError : 1;
		};
		uint8_t Raw;
	} PS2_STATUSES;

	typedef union
	{
		struct
		{
			uint8_t Port1Interrupt : 1;
			uint8_t Port2Interrupt : 1;
			uint8_t SystemFlag : 1;
			uint8_t Zero0 : 1;
			uint8_t Port1Clock : 1;
			uint8_t Port2Clock : 1;
			uint8_t Port1Translation : 1;
			uint8_t Zero1 : 1;
		};
		uint8_t Raw;
	} PS2_CONFIGURATION;

	void PS2Wait(const bool Output)
	{
		int Timeout = 100000;
		while (Timeout--)
		{
			PS2_STATUSES Status = {.Raw = inb(PS2_STATUS)};

			if (Output)
			{
				if (Status.OutputBufferFull)
					return;
			}
			else
			{
				if (!Status.InputBufferFull)
					return;
			}
		}

		PS2_STATUSES Status = {.Raw = inb(PS2_STATUS)};
	}

	void PS2WriteCommand(uint8_t Command)
	{
		PS2Wait(false);
		outb(PS2_CMD, Command);
	}

	void PS2WriteData(uint8_t Data)
	{
		PS2Wait(false);
		outb(PS2_DATA, Data);
	}

	uint8_t PS2ReadData()
	{
		PS2Wait(true);
		return inb(PS2_DATA);
	}

	uint8_t PS2ReadStatus()
	{
		return inb(PS2_STATUS);
	}

	bool __CheckHypervisorBit()
	{
		uint32_t eax, ebx, ecx, edx;
		__cpuid(0x1, &eax, &ebx, &ecx, &edx);
		if (!(ecx & (1 << 31)))
			return false; /* Hypervisor not detected */
		return true;
	}

	bool __VMwareBackdoorHypervisors()
	{
		const char hv[13] = {0};
		uint32_t eax, ebx, ecx, edx;
		__cpuid(0x40000000, &eax, &ebx, &ecx, &edx);

		*(uint32_t *)hv = ebx;
		*(uint32_t *)(hv + 4) = ecx;
		*(uint32_t *)(hv + 8) = edx;

		if (__strcmp(hv, "VMwareVMware") != 0 &&
			__strcmp(hv, "KVMKVMKVM") != 0 &&
			__strcmp(hv, "TCGTCGTCGTCG") != 0)
		{
			return false;
		}
		return true;
	}

	bool IsVMwareBackdoorAvailable()
	{
		if (!__CheckHypervisorBit())
			return false;

		if (!__VMwareBackdoorHypervisors())
			return false;

		struct
		{
			union
			{
				uint32_t ax;
				uint32_t magic;
			};
			union
			{
				uint32_t bx;
				size_t size;
			};
			union
			{
				uint32_t cx;
				uint16_t command;
			};
			union
			{
				uint32_t dx;
				uint16_t port;
			};
			uint32_t si;
			uint32_t di;
		} cmd;

		cmd.si = cmd.di = 0;
		cmd.bx = ~0x564D5868;
		cmd.command = 0xA;
		cmd.magic = 0x564D5868;
		cmd.port = 0x5658;

		asmv("in %%dx, %0" : "+a"(cmd.ax), "+b"(cmd.bx),
			 "+c"(cmd.cx), "+d"(cmd.dx),
			 "+S"(cmd.si), "+D"(cmd.di));

		if (cmd.bx != 0x564D5868 ||
			cmd.ax == 0xFFFFFFFF)
			return false;
		return true;
	}

	static int OpenMessageChannel(ToolboxContext *ctx, uint32_t Protocol)
	{
		uintptr_t ax, bx, cx, dx, si = 0, di = 0;

		VM_PORT(MESSAGE_OPEN_CHANNEL,
				(Protocol | FLAG_COOKIE), si, di,
				0, VMWARE_MAGIC,
				ax, bx, cx, dx, si, di);

		if ((HighWord(cx) & STATUS_SUCCESS) == 0)
		{
			trace("Failed to open message channel %#lx", Protocol);
			return -EINVAL;
		}

		debug("Opened message channel %d (Protocol: %#lx)",
			  HighWord(dx), Protocol);
		ctx->ChannelID = (uint16_t)HighWord(dx);
		ctx->CookieHigh = si;
		ctx->CookieLow = di;
		return 0;
	}

	static void MessageClose(ToolboxContext *ctx)
	{
		uintptr_t ax, bx, cx, dx,
			si = ctx->CookieHigh,
			di = ctx->CookieLow;

		VM_PORT(MESSAGE_CLOSE_CHANNEL,
				0, si, di,
				ctx->ChannelID << 16,
				VMWARE_MAGIC,
				ax, bx, cx, dx, si, di);

		debug("Closed message channel %d", ctx->ChannelID);
	}

	static uintptr_t MessageSendHB(ToolboxContext *ctx,
								   const char *Message)
	{
		uintptr_t ax, bx, cx, dx,
			si = (uintptr_t)Message,
			di = ctx->CookieLow,
			bp = ctx->CookieHigh;

		uint32_t ChannelID = ctx->ChannelID << 16;
		size_t Size = strlen(Message);

		VM_PORT_HB_OUT((STATUS_SUCCESS << 16) | MESSAGE_HB_MSG,
					   Size, si, di,
					   VMWARE_HYPERVISOR_HB | ChannelID | VMWARE_HYPERVISOR_OUT,
					   VMWARE_MAGIC, bp,
					   ax, bx, cx, dx, si, di);

		return bx;
	}

	static uintptr_t MessageSendLB(ToolboxContext *ctx,
								   const char *Message)
	{
		uintptr_t ax, bx,
			cx = STATUS_SUCCESS << 16,
			dx, si, di;

		size_t Size = strlen(Message);
		while (Size &&
			   (HighWord(cx) & STATUS_SUCCESS))
		{
			uint32_t TotalBytes = MIN((uint32_t)Size, (uint32_t)4);
			uint32_t Word = 0;
			memcpy(&Word, Message, TotalBytes);
			Message += TotalBytes;

			si = ctx->CookieHigh;
			di = ctx->CookieLow;

			VM_PORT(MESSAGE_SEND_PAYLOAD,
					Word, si, di,
					ctx->ChannelID << 16,
					VMWARE_MAGIC,
					ax, bx, cx, dx, si, di);
		}

		return cx;
	}

	static uintptr_t MessageReceiveHB(ToolboxContext *ctx,
									  char *Buffer,
									  size_t BufferSize)
	{
		uintptr_t ax, bx, cx, dx,
			si = ctx->CookieHigh,
			di = (uintptr_t)Buffer,
			bp = ctx->CookieLow;

		uint32_t ChannelID = ctx->ChannelID << 16;

		VM_PORT_HB_IN((STATUS_SUCCESS << 16) | MESSAGE_HB_MSG,
					  BufferSize, si, di,
					  VMWARE_HYPERVISOR_HB | ChannelID | VMWARE_HYPERVISOR_OUT,
					  VMWARE_MAGIC, bp,
					  ax, bx, cx, dx, si, di);

		return bx;
	}

	static uintptr_t MessageReceiveLB(ToolboxContext *ctx,
									  char *Buffer,
									  size_t BufferSize)
	{
		uintptr_t ax, bx,
			cx = STATUS_SUCCESS << 16,
			dx, si, di;

		while (BufferSize)
		{
			uint32_t TotalBytes = MIN((uint32_t)BufferSize, (uint32_t)4);

			si = ctx->CookieHigh;
			di = ctx->CookieLow;

			VM_PORT(MESSAGE_RECV_PAYLOAD,
					STATUS_SUCCESS, si, di,
					ctx->ChannelID << 16,
					VMWARE_MAGIC,
					ax, bx, cx, dx, si, di);

			if ((HighWord(cx) & STATUS_SUCCESS) == 0)
				break;

			memcpy(Buffer, &bx, TotalBytes);
			Buffer += TotalBytes;
			BufferSize -= TotalBytes;
		}

		return cx;
	}

	static int MessageSend(ToolboxContext *ctx,
						   const char *Message)
	{
		uintptr_t ax, bx, cx, dx, si, di;
		size_t Size = strlen(Message);
		int Retries = 0;

		while (Retries < 2)
		{
			Retries++;
			si = ctx->CookieHigh;
			di = ctx->CookieLow;

			VM_PORT(MESSAGE_SEND_SIZE,
					Size, si, di,
					ctx->ChannelID << 16,
					VMWARE_MAGIC,
					ax, bx, cx, dx, si, di);

			if ((HighWord(cx) & STATUS_SUCCESS) == 0)
			{
				trace("Failed to send message size for \"%s\": %d",
					  Message, cx);
				return -EINVAL;
			}

			bool HighBand = (HighWord(cx) & STATUS_HB) != 0;
			if (HighBand)
				bx = MessageSendHB(ctx, Message);
			else
				bx = MessageSendLB(ctx, Message);

			int status = HighWord(bx);

			if ((status & STATUS_SUCCESS) != 0)
			{
				debug("Message \"%s\" sent", Message);
				return 0;
			}
			else if ((status & STATUS_CPT) == 0)
			{
				trace("Checkpoint occurred for message \"%s\"", Message);
				continue;
			}
			else
				break;
		}

		trace("Failed to send message \"%s\": %#lx", Message, bx);
		return -EINVAL;
	}

	static int MessageReceive(ToolboxContext *ctx,
							  char **Buffer,
							  size_t *BufferSize)
	{
		uintptr_t ax, bx, cx, dx, si, di;
		int Retries = 0;

		*Buffer = NULL;
		*BufferSize = 0;

		char *ReplyBuf = NULL;
		size_t ReplyBufPages = 0;
		size_t ReplySize = 0;
		while (Retries < 2)
		{
			Retries++;
			si = ctx->CookieHigh;
			di = ctx->CookieLow;

			VM_PORT(MESSAGE_RECV_SIZE,
					0, si, di,
					ctx->ChannelID << 16,
					VMWARE_MAGIC,
					ax, bx, cx, dx, si, di);

			if ((HighWord(cx) & STATUS_SUCCESS) == 0)
			{
				trace("Failed to receive message size: %d", cx);
				return -EINVAL;
			}
			else if ((HighWord(cx) & STATUS_DORECV) == 0)
			{
				debug("No message to receive");
				return -EAGAIN;
			}

			ReplySize = bx;

			if (ReplyBuf != NULL)
				DriverManager->FreeMemory(DriverID, ReplyBuf, ReplyBufPages);
			ReplyBufPages = ReplySize / 0x1000 + 1;
			ReplyBuf = (char *)DriverManager->AllocateMemory(DriverID, ReplyBufPages);

			bool HighBand = (HighWord(cx) & STATUS_HB) != 0;
			if (HighBand)
				bx = MessageReceiveHB(ctx, ReplyBuf, ReplySize);
			else
				bx = MessageReceiveLB(ctx, ReplyBuf, ReplySize);

			if ((HighWord(bx) & STATUS_SUCCESS) == 0)
			{
				if ((HighWord(bx) & STATUS_CPT) == 0)
				{
					trace("Checkpoint occurred for message payload");
					continue;
				}

				trace("Failed to receive message payload: %d", HighWord(bx));
				DriverManager->FreeMemory(DriverID, ReplyBuf, ReplyBufPages);
				return -EINVAL;
			}

			ReplyBuf[ReplySize] = '\0';

			si = ctx->CookieHigh;
			di = ctx->CookieLow;

			VM_PORT(MESSAGE_RECV_STATUS,
					STATUS_SUCCESS, si, di,
					ctx->ChannelID << 16,
					VMWARE_MAGIC,
					ax, bx, cx, dx, si, di);

			if ((HighWord(cx) & STATUS_SUCCESS) == 0)
			{
				if ((HighWord(cx) & STATUS_CPT) == 0)
				{
					trace("Retrying message receive");
					continue;
				}

				trace("Failed to receive message status: %d", HighWord(cx));
				DriverManager->FreeMemory(DriverID, ReplyBuf, ReplyBufPages);
				return -EINVAL;
			}

			break;
		}

		if (ReplyBuf == NULL)
		{
			trace("Failed to receive message");
			return -EINVAL;
		}

		*Buffer = ReplyBuf;
		*BufferSize = ReplySize;
		debug("Received message \"%s\"", ReplyBuf);
		return 0;
	}

	static int SendRPCI(ToolboxContext *, const char *Request)
	{
		ToolboxContext rpci_ctx = {};
		int status = OpenMessageChannel(&rpci_ctx, MESSAGE_RPCI);
		if (status < 0)
		{
			trace("Failed to open RPCI channel: %d", status);
			return status;
		}

		status = MessageSend(&rpci_ctx, Request);
		if (status < 0)
		{
			trace("Failed to send RPCI request: %d", status);
			return status;
		}

		MessageClose(&rpci_ctx);
		return 0;
	}

	int MsgEqual(const char *haystack, const char *needle)
	{
		return strstr(haystack, needle) == haystack;
	}

	static int DisplayGetSize(ToolboxContext *ctx)
	{
		if (ctx->TCLOChannel != -1)
			MessageClose(ctx);
		OpenMessageChannel(ctx, MESSAGE_TCLO);

		char EmptyBuffer[256] = {'\0'};
		MessageSend(ctx, EmptyBuffer);

		while (true)
		{
			/* FIXME: buf memory leak */
			char *buf;
			size_t len;

			int status = MessageReceive(ctx, &buf, &len);
			if (status == -EAGAIN)
			{
				v0::Sleep(DriverID, 1000);
				continue;
			}
			else if (status < 0)
			{
				trace("Failed to receive message");
				return 1;
			}

			buf[strlen(buf)] = '\0';
			if (MsgEqual(buf, "reset"))
			{
				if (MessageSend(ctx, "OK ATR toolbox") < 0)
					return 1;
			}
			else if (MsgEqual(buf, "ping"))
			{
				if (MessageSend(ctx, "OK ") < 0)
					return 1;
			}
			else if (MsgEqual(buf, "Capabilities_Register"))
			{
				SendRPCI(ctx, "tools.capability.resolution_set 1");
				SendRPCI(ctx, "tools.capability.resolution_server toolbox 1");
				SendRPCI(ctx, "tools.capability.display_topology_set 1");
				SendRPCI(ctx, "tools.capability.color_depth_set 1");
				SendRPCI(ctx, "tools.capability.resolution_min 0 0");
				SendRPCI(ctx, "tools.capability.unity 1");

				if (MessageSend(ctx, "OK ") < 0)
					return 1;
			}
			else if (MsgEqual(buf, "Resolution_Set"))
			{
				debug("%s", buf);
				if (MessageSend(ctx, "OK ") < 0)
					return 1;
				MessageClose(ctx);
				return 0;
			}
			else
			{
				if (MessageSend(ctx, "ERROR Unknown command") < 0)
					return 1;
			}
		}
	}

	pid_t dst_id = -1;
	pid_t dst_pid = -1;
	ToolboxContext *tb_ctx = NULL;
	void DisplayScaleThread()
	{
		/* sizeof ToolboxContext */
		tb_ctx = (ToolboxContext *)DriverManager->AllocateMemory(DriverID, 1);
		v0::Sleep(DriverID, 2000);

		while (true)
		{
			if (DisplayGetSize(tb_ctx) != 0)
				trace("Failed to scale display");
			v0::Sleep(DriverID, 1000);
		}
	}

	void CommandSend(VMwareCommand *cmd)
	{
		cmd->magic = VMWARE_MAGIC;
		cmd->port = VMWARE_PORT;
		asm volatile("in %%dx, %0"
					 : "+a"(cmd->ax), "+b"(cmd->bx),
					   "+c"(cmd->cx), "+d"(cmd->dx),
					   "+S"(cmd->si), "+D"(cmd->di));
	}

	void Absolute()
	{
		VMwareCommand cmd = {};

		/* Enable */
		cmd.bx = ABSPOINTER_ENABLE;
		cmd.command = CMD_ABSPOINTER_COMMAND;
		CommandSend(&cmd);

		/* Status */
		cmd.bx = 0;
		cmd.command = CMD_ABSPOINTER_STATUS;
		CommandSend(&cmd);

		/* Read data (1) */
		cmd.bx = 1;
		cmd.command = CMD_ABSPOINTER_DATA;
		CommandSend(&cmd);

		/* Enable absolute */
		cmd.bx = ABSPOINTER_ABSOLUTE;
		cmd.command = CMD_ABSPOINTER_COMMAND;
		CommandSend(&cmd);
	}

	void Relative()
	{
		VMwareCommand cmd = {};
		cmd.bx = ABSPOINTER_RELATIVE;
		cmd.command = CMD_ABSPOINTER_COMMAND;
		CommandSend(&cmd);
	}

	InputReport ir = {};
	void InterruptHandler(CPU::TrapFrame *)
	{
		uint8_t Data = inb(0x60);
		(void)Data;

		VMwareCommand cmd = {};
		cmd.bx = 0;
		cmd.command = CMD_ABSPOINTER_STATUS;
		CommandSend(&cmd);

		if (cmd.ax == 0xFFFF0000)
		{
			trace("VMware mouse is not connected?");
			Relative();
			Absolute();
			return;
		}

		if ((cmd.ax & 0xFFFF) < 4)
			return;

		cmd.bx = 4;
		cmd.command = CMD_ABSPOINTER_DATA;
		CommandSend(&cmd);

		int Buttons = (cmd.ax & 0xFFFF);

		/**
		 * How should I handle this?
		 * (cmd.[bx,cx] * Width) / 0xFFFF
		 * Maybe TODO: Width and Height API?
		 */
		uintptr_t AbsoluteX = cmd.bx;
		uintptr_t AbsoluteY = cmd.cx;

		ir.Type = INPUT_TYPE_MOUSE;
		ir.Device = MouseDevID;
		ir.Mouse.X = AbsoluteX;
		ir.Mouse.Y = AbsoluteY;
		ir.Mouse.Z = (int8_t)cmd.dx;
		ir.Mouse.Absolute = 1;
		ir.Mouse.LeftButton = Buttons & 0x20;
		ir.Mouse.RightButton = Buttons & 0x10;
		ir.Mouse.MiddleButton = Buttons & 0x08;
		// ir.Mouse.Button4 = 0x0;
		// ir.Mouse.Button5 = 0x0;
		// ir.Mouse.Button6 = 0x0;
		// ir.Mouse.Button7 = 0x0;
		// ir.Mouse.Button8 = 0x0;
		v0::ReportInputEvent(DriverID, &ir);
	}

	int Ioctl(struct Inode *, unsigned long Request, void *)
	{
		switch (Request)
		{
		case 0x1:
			Relative();
			break;
		case 0x2:
			Absolute();
			break;
		default:
			return -EINVAL;
		}
		return 0;
	}

	const struct InodeOperations ops = {
		.Lookup = nullptr,
		.Create = nullptr,
		.Remove = nullptr,
		.Rename = nullptr,
		.Read = nullptr,
		.Write = nullptr,
		.Truncate = nullptr,
		.Open = nullptr,
		.Close = nullptr,
		.Ioctl = Ioctl,
		.ReadDir = nullptr,
		.MkDir = nullptr,
		.RmDir = nullptr,
		.SymLink = nullptr,
		.ReadLink = nullptr,
		.Seek = nullptr,
		.Stat = nullptr,
	};

	bool ToolboxSupported = false;
	int Entry()
	{
		ToolboxContext tb_ctx = {};
		/* Test if it's supported */
		int status = OpenMessageChannel(&tb_ctx, MESSAGE_TCLO);
		if (status == 0)
		{
			ToolboxSupported = true;
			MessageClose(&tb_ctx);
			dst_id = v0::CreateKernelThread(DriverID, 0, "VMware Display Scale",
											(void *)DisplayScaleThread, NULL);
			dst_pid = v0::GetCurrentProcess(DriverID);
		}

		PS2WriteCommand(PS2_CMD_ENABLE_PORT_2);
		PS2WriteCommand(PS2_CMD_READ_CONFIG);
		PS2_CONFIGURATION config = {.Raw = PS2ReadData()};
		config.Port2Interrupt = 1;
		PS2WriteCommand(PS2_CMD_WRITE_CONFIG);
		PS2WriteData(config.Raw);

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_SET_DEFAULTS);
		PS2ReadData();

		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_ENABLE_DATA_REPORTING);
		PS2ReadData();
		Absolute();

		/**
		 * If we have another driver using the PS/2 mouse, we need to
		 * override its interrupt handler.
		 */
		v0::OverrideInterruptHandler(DriverID, 12, (void *)InterruptHandler);

		MouseDevID = v0::RegisterDevice(DriverID, INPUT_TYPE_MOUSE, &ops);
		return 0;
	}

	int Final()
	{
		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_DISABLE_DATA_REPORTING);

		Relative();

		v0::UnregisterDevice(DriverID, MouseDevID);

		if (ToolboxSupported)
		{
			v0::KillThread(DriverID, dst_id, dst_pid, 0);
			if (tb_ctx->TCLOChannel != -1)
				MessageClose(tb_ctx);
			DriverManager->FreeMemory(DriverID, tb_ctx, 1);
		}
		return 0;
	}

	int Panic()
	{
		Relative();
		PS2WriteCommand(PS2_CMD_WRITE_NEXT_BYTE_TO_PS2_PORT_2_INPUT);
		PS2WriteData(PS2_MOUSE_CMD_DISABLE_DATA_REPORTING);
		return 0;
	}

	int Probe()
	{
		if (!IsVMwareBackdoorAvailable())
			return -ENODEV;
		return 0;
	}

	REGISTER_BUILTIN_DRIVER(vmware,
							"VMware Tools Driver",
							"enderice2",
							1, 0, 0,
							Entry,
							Final,
							Panic,
							Probe);
}

#endif
