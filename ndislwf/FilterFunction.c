#include "FilterFunction.h"

UINT32 netmask = 0x00ffffff;

BOOLEAN IsIpv4(_In_ struct ETH* eth)
{
    if (ntohs(eth->type) == 0x0800) // IPv4
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN IsLocalnetworkPacket(_In_ struct IP* ip)
{
    //if (!((((UINT32*)ip->dstIp)[0] & netmask) ^ (((UINT32*)ip->srcIp)[0] & netmask)))
    if (ip->srcIp[0] == ip->dstIp[0] &&
        ip->srcIp[1] == ip->dstIp[1] &&
        ip->srcIp[2] == ip->dstIp[2])
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN IsTargetPacket(_In_ BYTE* packet, _Outptr_ ULONG* frameSize)
{
    BOOLEAN result = FALSE;
    packet += sizeof(struct ETH);
    *frameSize += sizeof(struct ETH);

    if (((struct IP*)packet)->ip_p == 0x6)  // TCP packet;
    {
        *frameSize += ((struct IP*)packet)->ip_hl * 4;
        packet += ((struct IP*)packet)->ip_hl * 4;
        *frameSize += ((struct TCP*)packet)->th_off * 4;    // 
        result = TRUE;
    }
    else if (((struct IP*)packet)->ip_p == 0x11)    // UDP packet;
    {
        *frameSize += ((struct IP*)packet)->ip_hl * 4;
        *frameSize += 8;    // UDP header size
        result = TRUE;
    }

    return result;
}

VOID CopyPacketFromNetBufferLists(_In_ BYTE* buffer, _In_ PNET_BUFFER_LIST NetBufferLists, _In_ ULONG frameSize, _In_ ULONG packetSize, _In_ enum Direction direction)
{
    PMDL mdl = NetBufferLists->FirstNetBuffer->MdlChain;
    ULONG offset = NetBufferLists->FirstNetBuffer->DataOffset;

    if (direction == send)
    {
        NdisMoveMemory(buffer, (BYTE*)mdl->MappedSystemVa + offset, frameSize);
        buffer += frameSize;
        packetSize -= frameSize;

        while (mdl->Next && packetSize)
        {
            mdl = mdl->Next;
            NdisMoveMemory(buffer, (BYTE*)mdl->MappedSystemVa, mdl->ByteCount);
            buffer += mdl->ByteCount;
            packetSize = packetSize < mdl->ByteCount ? 0 : packetSize - mdl->ByteCount;
        }
    }
    else if (direction == receive)
    {
        NdisMoveMemory(buffer, (BYTE*)mdl->MappedSystemVa + offset, packetSize);
    }
}

VOID CopyPacketFromBuffer(_In_ PNET_BUFFER_LIST NetBufferLists, _In_ BYTE* buffer, _In_ ULONG frameSize, _In_ ULONG packetSize, _In_ enum Direction direction)
{
    PMDL mdl = NetBufferLists->FirstNetBuffer->MdlChain;
    ULONG offset = NetBufferLists->FirstNetBuffer->DataOffset;

    if (direction == send)
    {
        NdisMoveMemory((BYTE*)mdl->MappedSystemVa + offset, buffer, frameSize);
        buffer += frameSize;
        packetSize -= frameSize;

        while (mdl->Next && mdl->Size)
        {
            mdl = mdl->Next;
            NdisMoveMemory((BYTE*)mdl->MappedSystemVa, buffer, mdl->ByteCount);
            buffer += mdl->ByteCount;
            packetSize = packetSize < mdl->ByteCount ? 0 : packetSize - mdl->ByteCount;
        }
    }
    else if (direction == receive)
    {
        NdisMoveMemory((BYTE*)mdl->MappedSystemVa + offset, buffer, packetSize);
    }
}

VOID DeliverPacket(_In_ struct PacketInfo* packetInfo)
{
    PMS_FILTER pFilter = NULL;

    if (FilterModuleList.Flink != &FilterModuleList)
    {
        pFilter = CONTAINING_RECORD(FilterModuleList.Flink, MS_FILTER, FilterModuleLink);

        if (packetInfo->direction == send)
        {
            if (KeGetCurrentIrql() == DISPATCH_LEVEL)
            {
                packetInfo->Flags |= NDIS_SEND_FLAGS_DISPATCH_LEVEL;
            }
            else
            {
                packetInfo->Flags &= ~NDIS_SEND_FLAGS_DISPATCH_LEVEL;
            }
            NdisFSendNetBufferLists(pFilter->FilterHandle,
                packetInfo->NetBufferLists,
                packetInfo->PortNumber,
                packetInfo->Flags);
        }
        else if (packetInfo->direction == receive)
        {
            if (KeGetCurrentIrql() == DISPATCH_LEVEL)
            {
                packetInfo->Flags |= NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL;
            }
            else
            {
                packetInfo->Flags &= ~NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL;
            }
            NdisFIndicateReceiveNetBufferLists(
                pFilter->FilterHandle,
                packetInfo->NetBufferLists,
                packetInfo->PortNumber,
                packetInfo->NumberOfNetBufferLists,
                packetInfo->Flags);
        }
    }
}

/*NTSTATUS
FilterDeviceWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pWriteIrp
)
{
    PIO_STACK_LOCATION         pIrpSp;
    PMS_FILTER                 pFilter = NULL;
    PW32N_OPEN_CONTEXT         pW32NOpenContext = NULL;
    ULONG                      DataLength;
    PNET_BUFFER_LIST           pNetBufferList;
    PNET_BUFFER_LIST_CONTEXT   Context;
    PSEND_NETBUFLIST_RSVD      pSendRsvd;
    ULONG                      Flags = 0;
    PMDL                       pWriteMdl = NULL;

    Ndf_DbgPrint(DL_TRACE, DBG_WRITE, "==>FilterDeviceWrite\n");

    //
    // Initialize Default Settings
    //
    pWriteIrp->IoStatus.Information = 0;

    //
    // Get a pointer to the current stack location in the pWriteIrp. This is
    // where the function codes and parameters are located.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pWriteIrp);

    //
    // Find the Win32 Open Context
    //
    if (pIrpSp->FileObject && pIrpSp->FileObject->FsContext &&
        pIrpSp->FileObject->FsContext2)
    {
        pFilter = (PMS_FILTER)pIrpSp->FileObject->FsContext;

        pW32NOpenContext = W32N_MapOpenRefnum(
            pFilter,
            (ULONG_PTR)pIrpSp->FileObject->FsContext2
        );
    }

    //
    // Sanity Check on Win32 Context
    //
    if (!pW32NOpenContext)
    {
        //
        // Fail The IRP
        //
        pWriteIrp->IoStatus.Status = STATUS_INVALID_HANDLE;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_INVALID_HANDLE);
    }

    if (pW32NOpenContext->m_bIsClosing)
    {
        //
        // Fail The IRP
        //
        pWriteIrp->IoStatus.Status = STATUS_CANCELLED;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_CANCELLED);
    }

    DataLength = pIrpSp->Parameters.Write.Length;

    if (DataLength < ETHER_HDR_LEN)
    {
        Ndf_DbgPrint(DL_WARNING, DBG_WRITE, "FilterDeviceWrite: Too small to be a valid packet(%d bytes)\n", DataLength);

            // Fail The IRP
            pWriteIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_BUFFER_TOO_SMALL);     // Win32:
        ERROR_INSUFFICIENT_BUFFER
    }

    NDF_ACQUIRE_SPIN_LOCK(&pFilter->Lock, FALSE);

    //
    // Sanity Check on Filter State
    //
    if (pFilter->State != FilterRunning)
    {
        NDF_RELEASE_SPIN_LOCK(&pFilter->Lock, FALSE);

        Ndf_DbgPrint(DL_WARNING, DBG_WRITE, "FilterDeviceWrite: Filter Not Running!!!\n");

            // Fail The IRP
            pWriteIrp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_DEVICE_NOT_READY);      // Win32: ERROR_NOT_READY
    }

    ASSERT(pFilter->UserSendNetBufferListPool != NULL);

    pWriteMdl = NdisAllocateMdl(
        pFilter->FilterHandle,
        pWriteIrp->AssociatedIrp.SystemBuffer,
        DataLength
    );

    if (!pWriteMdl)
    {
        NDF_RELEASE_SPIN_LOCK(&pFilter->Lock, FALSE);

        Ndf_DbgPrint(DL_WARNING, DBG_WRITE, "FilterDeviceWrite: Could Not Allocate MDL!!!\n");

            // Fail The IRP
            pWriteIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_INSUFFICIENT_RESOURCES);      // Win32: ERROR_NOT_READY
    }

    pWriteMdl->Next = NULL;    // No Chained MDLs Here!

    pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
        pFilter->UserSendNetBufferListPool,
        sizeof(SEND_NETBUFLIST_RSVD), //Request control offset delta
        0,           // back fill size
        pWriteMdl,
        0,          // Data offset
        DataLength);

    if (pNetBufferList == NULL)
    {
        NDF_RELEASE_SPIN_LOCK(&pFilter->Lock, FALSE);

        Ndf_DbgPrint(DL_WARNING, DBG_WRITE, "FilterDeviceWrite: Binding %p: Failed to alloc send net buffer list\n", pFilter);

        NdisFreeMdl(pWriteMdl);

        // Fail The IRP
        pWriteIrp->IoStatus.Status = STATUS_NO_MEMORY;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_NO_MEMORY);
    }

    //
    // Find and Initialize the Context Area in the NBL
    //
    Context = pNetBufferList->Context;

    ASSERT(Context);

    if (!Context)
    {
        NDF_RELEASE_SPIN_LOCK(&pFilter->Lock, FALSE);

        NdisFreeMdl(pWriteMdl);

        NdisFreeNetBufferList(pNetBufferList);

        Ndf_DbgPrint(DL_WARNING, DBG_WRITE, "FilterDeviceWrite: Binding %p: Context NULL\n", pFilter);

        // Fail The IRP
        pWriteIrp->IoStatus.Status = STATUS_NO_MEMORY;
        pWriteIrp->IoStatus.Information = 0;         // Nothing Returned

        IoCompleteRequest(pWriteIrp, IO_NO_INCREMENT);

        return(STATUS_NO_MEMORY);
    }

    pSendRsvd = (PSEND_NETBUFLIST_RSVD)&Context->ContextData[0];

    pSendRsvd->pFilter = pFilter;
    pSendRsvd->pW32NOpenContext = pW32NOpenContext;
    pSendRsvd->OriginID = ORIGIN_USER_APPLICATION;
    pSendRsvd->pIrp = pWriteIrp;
    pSendRsvd->DataLength = DataLength;

    //
    // Add Reference Count to Binding
    // ------------------------------
    // This reference will be removed when the send completes.
    //
    filterAddReference(pFilter);

    IoMarkIrpPending(pWriteIrp);

    pNetBufferList->SourceHandle = pFilter->FilterHandle;

    // Increment These Counts With Lock Held
    if (pW32NOpenContext->m_bIsVirtualAdapter)
        ++pFilter->OutstandingRcvs;
    else
        ++pFilter->OutstandingSends;

    NDF_RELEASE_SPIN_LOCK(&pFilter->Lock, FALSE);

    if (pW32NOpenContext->m_bIsVirtualAdapter)
    {
        // Write on Virtual Adapter Indicates a Receive to the Host
        pSendRsvd->Internal.ReturnPacketHandler = UserIoReturnNetBufferList;

        Flags = 0;

        NdisFIndicateReceiveNetBufferLists(
            pFilter->FilterHandle,
            pNetBufferList,
            NDIS_DEFAULT_PORT_NUMBER,
            1,
            Flags     // Receive Flags
        );
    }
    else
    {
        // Write on Lower Adapter Sends a Packet on the Network
        pSendRsvd->Internal.SendCompleteHandler = UserIoSendComplete;

        Flags |= NDIS_SEND_FLAGS_CHECK_FOR_LOOPBACK;

        //
        // Disable checksum offload on this packet
        // ---------------------------------------
        // From the DDK:
        //
        // "If the TCP/IP transport does not set either the IsIPv4 or IsIPv6
        flag,
            // the miniport driver should not perform checksum tasks on the
            packet."
            //
            NET_BUFFER_LIST_INFO(pNetBufferList, TcpIpChecksumNetBufferListInfo)
            = 0;

        NdisFSendNetBufferLists(
            pFilter->FilterHandle,
            pNetBufferList,
            NDIS_DEFAULT_PORT_NUMBER,
            Flags  // Send Flags
        );
    }

    return STATUS_PENDING;
}*/