#include "precomp.h"
#include "PacketManager.h"

LIST_ENTRY gPacketListHead;
FILTER_LOCK gPacketListLock;

#define __FILENUMBER    'FteN'

VOID InitializePacketList()
{
    InitializeListHead(&gPacketListHead);
    FILTER_INIT_LOCK(&gPacketListLock);
}

VOID ClearPacketList()
{
    PLIST_ENTRY listEntry;
    struct PacketInfo* packetInfo;

    FILTER_ACQUIRE_LOCK(&gPacketListLock, FALSE);
    while (!IsListEmpty(&gPacketListHead))
    {
        listEntry = RemoveHeadList(&gPacketListHead);
        packetInfo = CONTAINING_RECORD(listEntry, struct PacketInfo, listEntry);
        RemovePacket(packetInfo);
    }
    FILTER_RELEASE_LOCK(&gPacketListLock, FALSE);
}

VOID UninitializePacketList()
{
    ClearPacketList();
    RemoveHeadList(&gPacketListHead);
    FILTER_FREE_LOCK(&gPacketListLock);
}

BOOLEAN InsertTailPacket(_In_ NDIS_HANDLE ndisHandle,
    _In_ ULONG frameSize,
    _In_ ULONG packetSize,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ NDIS_PORT_NUMBER PortNumber,
    _In_ ULONG NumberOfNetBufferLists,
    _In_ ULONG Flags,
    _In_ BOOLEAN isDispatchLevel,
    _In_ enum Direction direction)
{
    BOOLEAN result = FALSE;
    struct PacketInfo* packetInfo = FILTER_ALLOC_MEM(ndisHandle, sizeof(struct PacketInfo));

    if (packetInfo)
    {
        packetInfo->frameSize = frameSize;
        packetInfo->packetSize = packetSize;
        packetInfo->direction = direction;
        packetInfo->NetBufferLists = NetBufferLists;
        packetInfo->PortNumber = PortNumber;
        packetInfo->NumberOfNetBufferLists = NumberOfNetBufferLists;
        packetInfo->Flags = Flags;

        InsertTailPacketInfo(packetInfo, isDispatchLevel);
        result = TRUE;
    }
    else
    {
        DbgPrint("packetInfo = FILTER_ALLOC_MEM(ndisHandle, sizeof(struct PacketInfo)); fail!!!!!\n");
    }

    return result;
}

VOID InsertTailPacketInfo(struct PacketInfo* packetInfo, _In_ BOOLEAN isDispatchLevel)
{
    FILTER_ACQUIRE_LOCK(&gPacketListLock, isDispatchLevel);
    InsertTailList(&gPacketListHead, &packetInfo->listEntry);
    FILTER_RELEASE_LOCK(&gPacketListLock, isDispatchLevel);
}

VOID RemovePacket(_Inout_ struct PacketInfo* packetInfo)
{
    FILTER_FREE_MEM(packetInfo);
}

BOOLEAN PeekPacket(_Outptr_ struct PacketInfo** packetInfo, _In_ BOOLEAN isDispatchLevel)
{
    BOOLEAN result = FALSE;

    FILTER_ACQUIRE_LOCK(&gPacketListLock, isDispatchLevel);
    if (!IsListEmpty(&gPacketListHead))
    {
        *packetInfo = CONTAINING_RECORD(gPacketListHead.Flink, struct PacketInfo, listEntry);
        result = TRUE;
    }
    FILTER_RELEASE_LOCK(&gPacketListLock, isDispatchLevel);

    return result;
}

BOOLEAN PopPacket(_Outptr_ struct PacketInfo** packetInfo, _In_ BOOLEAN isDispatchLevel)
{
    BOOLEAN result = FALSE;

    FILTER_ACQUIRE_LOCK(&gPacketListLock, isDispatchLevel);
    if (!IsListEmpty(&gPacketListHead))
    {
        *packetInfo = CONTAINING_RECORD(RemoveHeadList(&gPacketListHead), struct PacketInfo, listEntry);
        result = TRUE;
    }
    FILTER_RELEASE_LOCK(&gPacketListLock, isDispatchLevel);

    return result;
}