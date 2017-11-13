#pragma once

#include "precomp.h"
#include "PacketManager.h"

#define htonl(l)                  \
   ((((l) & 0xFF000000) >> 24) | \
   (((l) & 0x00FF0000) >> 8)  |  \
   (((l) & 0x0000FF00) << 8)  |  \
   (((l) & 0x000000FF) << 24))

#define htons(s) \
   ((((s) >> 8) & 0x00FF) | \
   (((s) << 8) & 0xFF00))

#define ntohl(l)                   \
   ((((l) >> 24) & 0x000000FFL) | \
   (((l) >>  8) & 0x0000FF00L) |  \
   (((l) <<  8) & 0x00FF0000L) |  \
   (((l) << 24) & 0xFF000000L))

#define ntohs(s)                     \
   ((USHORT)((((s) & 0x00ff) << 8) | \
   (((s) & 0xff00) >> 8)))

#define CONVERT_IP(x) (struct IP*)(x + sizeof(struct ETH))

struct PseudoHeader
{
    UINT32 srcIp;
    UINT32 dstIp;
    UCHAR reserved;
    UCHAR protocol;
    USHORT tcpLength;
};

BOOLEAN IsIpv4(_In_ struct ETH* eth);

BOOLEAN IsLocalnetworkPacket(_In_ struct IP* ip);

BOOLEAN IsTargetPacket(_In_ BYTE* packet, _Outptr_ ULONG* frameSize);

VOID CopyPacketFromNetBufferLists(_In_ BYTE* buffer, _In_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ ULONG packetSize, _In_ enum Direction direction);

VOID CopyPacketFromBuffer(_In_ PNET_BUFFER_LIST NetBufferLists, _In_ BYTE* buffer, _In_ ULONG frameSize, _In_ ULONG packetSize, _In_ enum Direction direction);

VOID DeliverPacket(struct PacketInfo* packetInfo);