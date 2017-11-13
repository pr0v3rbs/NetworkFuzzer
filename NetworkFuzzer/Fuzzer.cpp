#include "Fuzzer.h"
#include <random>

void Mutate(_Inout_ BYTE* packet, _In_ DWORD packetSize)
{
    for (int i = 0; i < (rand() % 10) + 1; i++)
        packet[rand() % packetSize] = (BYTE)rand();
}