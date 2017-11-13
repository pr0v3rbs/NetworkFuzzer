#include "Fuzzer.h"
#include "FilterFunction.h"

VOID Mutate(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG packetSize, _In_ ULONG frameSize, _In_ enum Direction direction)
{
    ULONG seed = frameSize;
    ULONG idxNum = RtlRandomEx(&seed) % 3;

    for (ULONG i = 0; i < idxNum + 1; i++)
    {
        //if ((RtlRandomEx(&seed) % 100) < 20)   // Set occurring fuzz percentage
        {
            ULONG idx = RtlRandomEx(&seed);
            /*if (direction == send)
            {
                idx %= (packetSize - frameSize);
                idx += frameSize;
            }
            else if (direction == receive)
            {
                idx %= packetSize;
            }*/
            idx %= (packetSize - frameSize);
            idx += frameSize;
            SetIndex(NetBufferLists,
                frameSize,
                idx,
                (BYTE)RtlRandomEx(&seed),
                direction);
        }
    }
}

VOID MutateFrame(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ enum Direction direction)
{
    ULONG seed = frameSize;
    ULONG idxNum = RtlRandomEx(&seed) % 3;

    for (ULONG i = 0; i < idxNum + 1; i++)
    {
        if ((RtlRandomEx(&seed) % 100) < 20)   // Set occurring fuzz percentage
        {
            ULONG idx = RtlRandomEx(&seed) % frameSize;
            SetIndex(NetBufferLists,
                frameSize,
                idx,
                (BYTE)RtlRandomEx(&seed),
                direction);
        }
    }
}

VOID SetIndex(_Inout_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ ULONG idx, _In_ UCHAR data, _In_ enum Direction direction)
{
    PMDL mdl = NetBufferLists->FirstNetBuffer->MdlChain;
    ULONG offset = NetBufferLists->FirstNetBuffer->DataOffset;

    if (direction == send)
    {
        if (idx < frameSize)
        {
            ((BYTE*)mdl->MappedSystemVa + offset)[idx] = data;
        }
        else
        {
            idx -= frameSize;

            while (mdl->Next && mdl->Size)
            {
                mdl = mdl->Next;
                if (idx < mdl->ByteCount)
                {
                    ((BYTE*)mdl->MappedSystemVa)[idx] = data;
                    break;
                }
                idx += mdl->ByteCount;
            }
        }
    }
    else if (direction == receive)
    {
        ((BYTE*)mdl->MappedSystemVa + offset)[idx] = data;
    }
}