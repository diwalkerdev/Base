#include "linux/platform.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <string.h>

struct DummyData
{
    uint64                x;
    uint64*               px;
    std::array<uint64, 4> block;
};

void
Test_VirtualMemory()
{
    void* memory;
    auto  FourKbytes = Kilobytes(4);
    {
        printf("CHECK - Can allocate virtual memory...\n");

        memory = Platform_AllocateVirtualMemory(FourKbytes);
        printf("Address: %p\n", memory);
        assert(memory != 0);
    }

    DummyData* data;
    {
        printf("CHECK - Can use virtual memory...\n");
        data           = Cast(DummyData*, memory);
        data->x        = 42;
        data->px       = &data->x;
        data->block[0] = 0;
        data->block[1] = 1;
        data->block[2] = 2;
        data->block[3] = 3;

        printf("X         : %lu\n", data->x);
        printf("X Address : %p\n", (void*)data->px);
        printf("Block[1]  : %lu\n", data->block[1]);

        assert((void*)data->px == memory);
        assert(data->block[1] == 1);
    }

    DummyData copy = *data;
    {
        printf("CHECK - Can reset virtual memory...\n");

        memset(memory, 0, sizeof(DummyData));

        printf("X         : %lu\n", data->x);
        printf("X Address : %p\n", (void*)data->px);
        printf("Block[1]  : %lu\n", data->block[1]);

        assert((void*)data->px == 0);
        assert(data->block[1] == 0);
    }

    {
        printf("CHECK - Can load to virtual memory...\n");
        data->x        = copy.x;
        data->px       = copy.px;
        data->block[0] = copy.block[0];
        data->block[1] = copy.block[1];
        data->block[2] = copy.block[2];
        data->block[3] = copy.block[3];

        printf("X         : %lu\n", data->x);
        printf("X Address : %p\n", (void*)data->px);
        printf("Block[1]  : %lu\n", data->block[1]);

        assert((void*)data->px == memory);
        assert(data->block[1] == 1);
    }


    Platform_FreeVirtualMemory(memory, FourKbytes);
    printf("TEST Test_VirtualMemory COMPLETE\n");
}
