#pragma once

enum Direction
{
    send,
    receive
};

struct PacketInfo
{
    LIST_ENTRY listEntry;
    ULONG frameSize;
    ULONG packetSize;
    enum Direction direction;
    PNET_BUFFER_LIST NetBufferLists;
    NDIS_PORT_NUMBER PortNumber;
    ULONG NumberOfNetBufferLists;
    ULONG Flags;
};

VOID InitializePacketList();

VOID ClearPacketList();

VOID UninitializePacketList();

BOOLEAN InsertTailPacket(_In_ NDIS_HANDLE ndisHandle,
    _In_ ULONG frameSize,
    _In_ ULONG packetSize,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ NDIS_PORT_NUMBER PortNumber,
    _In_ ULONG NumberOfNetBufferLists,
    _In_ ULONG Flags,
    _In_ BOOLEAN isDispatchLevel,
    _In_ enum Direction direction);

VOID InsertTailPacketInfo(struct PacketInfo* packetInfo, _In_ BOOLEAN isDispatchLevel);

VOID RemovePacket(_Inout_ struct PacketInfo* packetInfo);

BOOLEAN PeekPacket(_Outptr_ struct PacketInfo** packetInfo, _In_ BOOLEAN isDispatchLevel);

BOOLEAN PopPacket(_Outptr_ struct PacketInfo** packetInfo, _In_ BOOLEAN isDispatchLevel);