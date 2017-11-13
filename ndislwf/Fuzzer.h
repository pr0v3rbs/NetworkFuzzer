#pragma once

#include "precomp.h"
#include "PacketManager.h"

VOID Mutate(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG packetSize, _In_ ULONG frameSize, _In_ enum Direction direction);

VOID MutateFrame(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ enum Direction direction);

VOID SetIndex(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ ULONG idx, _In_ UCHAR data, _In_ enum Direction direction);