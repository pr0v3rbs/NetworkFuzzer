#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <process.h>
#include <ntddndis.h>
#include <winioctl.h>
#include <share.h>
#include <time.h>
#include <random>
#include "NetworkFuzzer.h"
#include "Fuzzer.h"
#include "../ndislwf/filteruser.h"

//#define DEBUG

TCHAR gDeviceName[] = _T("\\\\.\\NDISLWF");
HANDLE gDeviceHandle;
bool gThreadTerminated;

BOOL InitDevice()
{
    BOOL result = FALSE;

    gDeviceHandle = CreateFile(gDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (gDeviceHandle != INVALID_HANDLE_VALUE)
    {
        result = TRUE;
    }

    return result;
}

BOOL UninitDevice()
{
    return CloseHandle(gDeviceHandle);
}

BOOL DeviceFilterSetting(bool isEnable)
{
    DWORD deviceReturn;

    return DeviceIoControl(gDeviceHandle, IOCTL_FILTER_SETTING, &isEnable, sizeof(isEnable), NULL, 0, &deviceReturn, 0);
}

unsigned __stdcall ThreadFunction(PVOID parameter)
{
    UNREFERENCED_PARAMETER(parameter);

    BYTE buffer[10000];
    BYTE* packet;
    DWORD packetSize;
    DWORD frameSize;
    //DWORD mutatedPacketSize;

    printf("Thread Start!\n");
    srand((unsigned int)time(NULL));
    while (gThreadTerminated == FALSE)
    {
        if (DeviceIoControl(gDeviceHandle, IOCTL_FILTER_GET_PACKET, NULL, 0, buffer, sizeof(buffer), &packetSize, 0))
        {
            packet = buffer + sizeof(DWORD);
            frameSize = *(DWORD*)buffer;
            packetSize -= sizeof(DWORD);
#ifdef DEBUG
            printf("packet Size = %d\n", packetSize);
            printf("-----------------------packet-----------------------------\n");
            for (DWORD i = 0; i < packetSize; i++)
                printf("%02x ", (UCHAR)packet[i]);
            printf("\n-----------------------packet-----------------------------\n");
#endif // DEBUG
            if ((rand() % 5) < 1)
            {
                printf("Mutated!\n");
                Mutate(packet + frameSize, packetSize - frameSize);
                DeviceIoControl(gDeviceHandle, IOCTL_FILTER_SET_MUTATED_PACKET, packet, packetSize, NULL, 0, NULL, 0);
            }
            else
            {
                DeviceIoControl(gDeviceHandle, IOCTL_FILTER_SET_PACKET_UNMUTATED, NULL, 0, NULL, 0, NULL, 0);
            }
        }
        else
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                printf("buffer is too small!!!!\n");
            }
            Sleep(100);
        }
    }
    printf("Thread End!\n");

    return 0;
}

int main()
{
    char userInput[256];
    //HANDLE threadHandle = NULL;
    //unsigned threadID;

    if (InitDevice())
    {
        while (true)
        {
            printf("1. Filter Enable\n");
            printf("2. Filter Disable\n");
            printf("3. Exit\n");

            gets_s(userInput, 256);

            if (userInput[0] == '1')
            {
                DeviceFilterSetting(TRUE);
                /*if (threadHandle == NULL && DeviceFilterSetting(TRUE))
                {
                    gThreadTerminated = false;
                    threadHandle = (HANDLE)_beginthreadex(NULL, 0, &ThreadFunction, NULL, 0, &threadID);
                }*/
            }
            else if (userInput[0] == '2')
            {
                DeviceFilterSetting(FALSE);
                /*if (threadHandle && DeviceFilterSetting(FALSE))
                {
                    gThreadTerminated = true;
                    WaitForSingleObject(threadHandle, INFINITE);
                    threadHandle = NULL;
                }*/
            }
            else if (userInput[0] == '3')
            {
                // TODO: Disable filter.
                UninitDevice();
                break;
            }
        }
    }
    else
    {
        printf("Init Device Fail!\n");
    }

    return 0;
}