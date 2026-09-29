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

typedef EFI_STATUS(EFIAPI *EFI_INPUT_RESET)(
	IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
	IN BOOLEAN ExtendedVerification);

typedef EFI_STATUS(EFIAPI *EFI_INPUT_READ_KEY)(
	IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
	OUT EFI_INPUT_KEY *Key);

typedef EFI_TPL(EFIAPI *EFI_RAISE_TPL)(
	IN EFI_TPL NewTpl);

typedef VOID(EFIAPI *EFI_RESTORE_TPL)(
	IN EFI_TPL OldTpl);

typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_PAGES)(
	IN EFI_ALLOCATE_TYPE Type,
	IN EFI_MEMORY_TYPE MemoryType,
	IN UINTN Pages,
	IN OUT EFI_PHYSICAL_ADDRESS *Memory);

typedef EFI_STATUS(EFIAPI *EFI_FREE_PAGES)(
	IN EFI_PHYSICAL_ADDRESS Memory,
	IN UINTN Pages);

typedef EFI_STATUS(EFIAPI *EFI_GET_MEMORY_MAP)(
	IN OUT UINTN *MemoryMapSize,
	IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
	OUT UINTN *MapKey,
	OUT UINTN *DescriptorSize,
	OUT UINT32 *DescriptorVersion);

typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_POOL)(
	IN EFI_MEMORY_TYPE PoolType,
	IN UINTN Size,
	OUT VOID **Buffer);

typedef EFI_STATUS(EFIAPI *EFI_FREE_POOL)(
	IN VOID *Buffer);

typedef VOID(EFIAPI *EFI_EVENT_NOTIFY)(
	IN EFI_EVENT Event,
	IN VOID *Context);

typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT)(
	IN UINT32 Type,
	IN EFI_TPL NotifyTpl,
	IN EFI_EVENT_NOTIFY NotifyFunction,
	IN VOID *NotifyContext,
	OUT EFI_EVENT *Event);

typedef EFI_STATUS(EFIAPI *EFI_SET_TIMER)(
	IN EFI_EVENT Event,
	IN EFI_TIMER_DELAY Type,
	IN UINT64 TriggerTime);

typedef EFI_STATUS(EFIAPI *EFI_WAIT_FOR_EVENT)(
	IN UINTN NumberOfEvents,
	IN EFI_EVENT *Event,
	OUT UINTN *Index);

typedef EFI_STATUS(EFIAPI *EFI_SIGNAL_EVENT)(
	IN EFI_EVENT Event);

typedef EFI_STATUS(EFIAPI *EFI_CLOSE_EVENT)(
	IN EFI_EVENT Event);

typedef EFI_STATUS(EFIAPI *EFI_CHECK_EVENT)(
	IN EFI_EVENT Event);

typedef EFI_STATUS(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(
	IN OUT EFI_HANDLE *Handle,
	IN EFI_GUID const *Protocol,
	IN EFI_INTERFACE_TYPE InterfaceType,
	IN VOID *Interface);

typedef EFI_STATUS(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	IN VOID *OldInterface,
	IN VOID *NewInterface);

typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	IN VOID *Interface);

typedef EFI_STATUS(EFIAPI *EFI_HANDLE_PROTOCOL)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	OUT VOID **Interface);

typedef EFI_STATUS(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
	IN EFI_GUID const *Protocol,
	IN EFI_EVENT Event,
	OUT VOID **Registration);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE)(
	IN EFI_LOCATE_SEARCH_TYPE SearchType,
	IN EFI_GUID const *Protocol, OPTIONAL IN VOID *SearchKey, OPTIONAL IN OUT UINTN *BufferSize,
	OUT EFI_HANDLE *Buffer);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_DEVICE_PATH)(
	IN EFI_GUID const *Protocol,
	IN OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath,
	OUT EFI_HANDLE *Device);

typedef EFI_STATUS(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
	IN EFI_GUID const *Guid,
	IN VOID *Table);

typedef EFI_STATUS(EFIAPI *EFI_IMAGE_LOAD)(
	IN BOOLEAN BootPolicy,
	IN EFI_HANDLE ParentImageHandle,
	IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
	IN VOID *SourceBuffer OPTIONAL,
	IN UINTN SourceSize,
	OUT EFI_HANDLE *ImageHandle);

typedef EFI_STATUS(EFIAPI *EFI_IMAGE_START)(
	IN EFI_HANDLE ImageHandle,
	OUT UINTN *ExitDataSize,
	OUT CHAR16 **ExitData OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_EXIT)(
	IN EFI_HANDLE ImageHandle,
	IN EFI_STATUS ExitStatus,
	IN UINTN ExitDataSize,
	IN CHAR16 *ExitData OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_IMAGE_UNLOAD)(
	IN EFI_HANDLE ImageHandle);

typedef EFI_STATUS(EFIAPI *EFI_EXIT_BOOT_SERVICES)(
	IN EFI_HANDLE ImageHandle,
	IN UINTN MapKey);

typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(
	OUT UINT64 *Count);

typedef EFI_STATUS(EFIAPI *EFI_STALL)(
	IN UINTN Microseconds);

typedef EFI_STATUS(EFIAPI *EFI_SET_WATCHDOG_TIMER)(
	IN UINTN Timeout,
	IN UINT64 WatchdogCode,
	IN UINTN DataSize,
	IN CHAR16 *WatchdogData OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_CONNECT_CONTROLLER)(
	IN EFI_HANDLE ControllerHandle,
	IN EFI_HANDLE *DriverImageHandle, OPTIONAL IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath, OPTIONAL IN BOOLEAN Recursive);

typedef EFI_STATUS(EFIAPI *EFI_DISCONNECT_CONTROLLER)(
	IN EFI_HANDLE ControllerHandle,
	IN EFI_HANDLE DriverImageHandle, OPTIONAL IN EFI_HANDLE ChildHandle OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	OUT VOID **Interface, OPTIONAL IN EFI_HANDLE AgentHandle,
	IN EFI_HANDLE ControllerHandle,
	IN UINT32 Attributes);

typedef EFI_STATUS(EFIAPI *EFI_CLOSE_PROTOCOL)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	IN EFI_HANDLE AgentHandle,
	IN EFI_HANDLE ControllerHandle);

typedef EFI_STATUS(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
	IN EFI_HANDLE Handle,
	IN EFI_GUID const *Protocol,
	OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
	OUT UINTN *EntryCount);

typedef EFI_STATUS(EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
	IN EFI_HANDLE Handle,
	OUT EFI_GUID const ***ProtocolBuffer,
	OUT UINTN *ProtocolBufferCount);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
	IN EFI_LOCATE_SEARCH_TYPE SearchType,
	IN EFI_GUID const *Protocol, OPTIONAL IN VOID *SearchKey, OPTIONAL IN OUT UINTN *NoHandles,
	OUT EFI_HANDLE **Buffer);

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_PROTOCOL)(
	IN EFI_GUID const *Protocol,
	IN VOID *Registration, OPTIONAL OUT VOID **Interface);

typedef EFI_STATUS(EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
	IN OUT EFI_HANDLE *Handle,
	...);

typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
	IN EFI_HANDLE Handle,
	...);

typedef EFI_STATUS(EFIAPI *EFI_CALCULATE_CRC32)(
	IN VOID *Data,
	IN UINTN DataSize,
	OUT UINT32 *Crc32);

typedef VOID(EFIAPI *EFI_COPY_MEM)(
	IN VOID *Destination,
	IN VOID *Source,
	IN UINTN Length);

typedef VOID(EFIAPI *EFI_SET_MEM)(
	IN VOID *Buffer,
	IN UINTN Size,
	IN UINT8 Value);

typedef EFI_STATUS(EFIAPI *EFI_GET_TIME)(
	OUT EFI_TIME *Time,
	OUT EFI_TIME_CAPABILITIES *Capabilities OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_SET_TIME)(
	IN EFI_TIME *Time);

typedef EFI_STATUS(EFIAPI *EFI_GET_WAKEUP_TIME)(
	OUT BOOLEAN *Enabled,
	OUT BOOLEAN *Pending,
	OUT EFI_TIME *Time);

typedef EFI_STATUS(EFIAPI *EFI_SET_WAKEUP_TIME)(
	IN BOOLEAN Enable,
	IN EFI_TIME *Time OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(
	IN UINTN MemoryMapSize,
	IN UINTN DescriptorSize,
	IN UINT32 DescriptorVersion,
	IN EFI_MEMORY_DESCRIPTOR *VirtualMap);

typedef EFI_STATUS(EFIAPI *EFI_CONVERT_POINTER)(
	IN UINTN DebugDisposition,
	IN OUT VOID **Address);

typedef EFI_STATUS(EFIAPI *EFI_GET_VARIABLE)(
	IN CHAR16 *VariableName,
	IN EFI_GUID *VendorGuid,
	OUT UINT32 *Attributes OPTIONAL,
	IN OUT UINTN *DataSize,
	OUT VOID *Data);

typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(
	IN OUT UINTN *VariableNameSize,
	IN OUT CHAR16 *VariableName,
	IN OUT EFI_GUID *VendorGuid);

typedef EFI_STATUS(EFIAPI *EFI_SET_VARIABLE)(
	IN CHAR16 *VariableName,
	IN EFI_GUID *VendorGuid,
	IN UINT32 Attributes,
	IN UINTN DataSize,
	IN VOID *Data);

typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(
	OUT UINT32 *HighCount);

typedef EFI_STATUS(EFIAPI *EFI_RESET_SYSTEM)(
	IN EFI_RESET_TYPE ResetType,
	IN EFI_STATUS ResetStatus,
	IN UINTN DataSize,
	IN CHAR16 *ResetData OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_UPDATE_CAPSULE)(
	IN EFI_CAPSULE_HEADER **CapsuleHeaderArray,
	IN UINTN CapsuleCount,
	IN EFI_PHYSICAL_ADDRESS ScatterGatherList OPTIONAL);

typedef EFI_STATUS(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES)(
	IN EFI_CAPSULE_HEADER **CapsuleHeaderArray,
	IN UINTN CapsuleCount,
	OUT UINT64 *MaximumCapsuleSize,
	OUT EFI_RESET_TYPE *ResetType);

typedef EFI_STATUS(EFIAPI *EFI_QUERY_VARIABLE_INFO)(
	IN UINT32 Attributes,
	OUT UINT64 *MaximumVariableStorageSize,
	OUT UINT64 *RemainingVariableStorageSize,
	OUT UINT64 *MaximumVariableSize);
