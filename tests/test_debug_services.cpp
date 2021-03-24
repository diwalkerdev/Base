#include "debug_services.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <limits.h>
#include <stdio.h>

void
UnusedFunctionShouldNotGetPrintedAsItIsNotCalled()
{
    // Time blocks get allocated based on the value of __COUNTER__ and get allocated regardless to if the function is called or not.
    // Therefore it is necessary to check all items when iterating over a store.
    TIME_BLOCK;
}

void
DoNothing()
{
    TIME_BLOCK;
}

// Basically just a short form of Debug_TimeBlockRecord_Count.
uint32
CallCount(Debug_TimeBlockRecord const& record)
{
    return Debug_TimeBlockRecord_Count(record);
}

void
TakesAboutASecond()
{
    TIME_BLOCK;
    SDL_Delay(1000);
}

void
Test_TimeBlockUsageExample()
{
    printf("TEST Test_TimeBlockUsageExample...\n");

    // Manual initialisation gives control on the order of how things are initialised. This is slightly less
    // convenient then allocating as a global without new but helps prevent against static order initialisation fiasco.
    DEBUG_INIT_GLOBAL_DEBUG_SERVICES();

    {
        // It is possible to have multiple TIME_BLOCKS in the same scope.
        // It is NOT possible to have multple TIME_BLOCKS on the same line (as the variable name generated will be identical).
        TIME_BLOCK;
        TIME_BLOCK;
        {
            TIME_BLOCK;
            DoNothing();
            DoNothing();
        }
        TakesAboutASecond();
    }

    // Records are stored per file.
    auto const& records = global_debug_time_block_store->file_name_to_records_map[__FILE_NAME__];

#ifdef DEBUG_BUILD
    assert(CallCount(records[0]) == 0); // unused function
    assert(CallCount(records[1]) == 2); // check_something
    assert(CallCount(records[2]) == 1); // example block 1
    assert(CallCount(records[3]) == 1); // example block 2
    assert(CallCount(records[4]) == 1); // example block 3
    assert(CallCount(records[5]) == 1); // about a second
    assert(CallCount(records[6]) == 0); // past last block
#else
    // Make sure nothing happens in a release build.
    assert(CallCount(records[0]) == 0); // unused function
    assert(CallCount(records[1]) == 0); // check_something
    assert(CallCount(records[2]) == 0); // example block 1
    assert(CallCount(records[3]) == 0); // example block 2
    assert(CallCount(records[4]) == 0); // example block 3
    assert(CallCount(records[5]) == 0); // about a second
    assert(CallCount(records[6]) == 0); // past last block
#endif

    DEBUG_PRINT_TIME_BLOCK_RECORDS(global_debug_time_block_store);

    DEBUG_RESET_TIME_BLOCK_RECORDS(global_debug_time_block_store);
    assert(CallCount(records[0]) == 0); // unused function
    assert(CallCount(records[1]) == 0); // check_something
    assert(CallCount(records[2]) == 0); // example block 1
    assert(CallCount(records[3]) == 0); // example block 2
    assert(CallCount(records[4]) == 0); // example block 3
    assert(CallCount(records[5]) == 0); // about a second
    assert(CallCount(records[6]) == 0); // past last block

    // DEBUG_PRINT_TIME_BLOCK_RECORDS(global_debug_time_block_store);  // Won't print anything after a reset.

    // Remember to free debug services.
    DEBUG_FREE_GLOBAL_DEBUG_SERVICES();
}

struct GameStruct
{
    uint8 x;
};

void
Test_RecordAndPlaybackFromFile()
{
    printf("TEST Test_RecordAndPlayback from file...\n");

    GameStruct game_struct;
    auto*      file_name = "test_recording.bin";
    int64      end;
    {
        auto* io = Debug_RecordingStartFromFile(file_name);
        assert(io != nullptr);

        game_struct.x = 1;
        Debug_RecordingWrite(io, &game_struct);
        game_struct.x = 2;
        Debug_RecordingWrite(io, &game_struct);
        game_struct.x = 3;
        Debug_RecordingWrite(io, &game_struct);

        end = Debug_RecordingStop(io);
        assert(end == 3);
    }

    {
        auto* io = Debug_PlaybackStartFromFile(file_name);
        assert(io != nullptr);

        for (int loop = 0; loop < 4; ++loop)
        {
            Debug_PlaybackRead(io, &game_struct);
            assert(game_struct.x == 1);

            Debug_PlaybackRead(io, &game_struct);
            assert(game_struct.x == 2);

            Debug_PlaybackRead(io, &game_struct);
            assert(game_struct.x == 3);
        }

        Debug_PlaybackStop(io);
    }
}

void
Test_RecordAndPlaybackFromMemory()
{
    printf("TEST Test_RecordAndPlayback from memory...\n");
    GameStruct game_struct;
    char       memory[4];
    int64      end;

    {
        auto* io = Debug_RecordingStartFromMemory((void*)memory, sizeof(memory));
        assert(io != nullptr);

        game_struct.x = 1;
        Debug_RecordingWrite(io, &game_struct);
        game_struct.x = 2;
        Debug_RecordingWrite(io, &game_struct);
        game_struct.x = 3;
        Debug_RecordingWrite(io, &game_struct);

        // If you try to overflow the memory region 0 bytes will get written,
        // game_struct.x = 4;
        // Debug_RecordingWrite(io, &game_struct);

        // game_struct.x = 5;
        // auto bytes = Debug_RecordingWrite(io, &game_struct);
        // assert(bytes == 0);

        assert(SDL_RWsize(io) == 4); // Check before stop otherwise is null.
        end = Debug_RecordingStop(io);
        assert(end == 3);
    }

    {
        auto* io = Debug_PlaybackStartFromMemory((void*)memory, sizeof(char) * 4);
        assert(io != nullptr); // game_struct.x = 4;
        // Debug_RecordingWrite(io, &game_struct);

        // game_struct.x = 5;
        // auto bytes = Debug_RecordingWrite(io, &game_struct);
        // assert(bytes == 0);

        for (int loop = 0; loop < 4; ++loop)
        {
            Debug_PlaybackRead(io, &game_struct, end);
            assert(game_struct.x == 1);

            Debug_PlaybackRead(io, &game_struct, end);
            assert(game_struct.x == 2);

            Debug_PlaybackRead(io, &game_struct, end);
            assert(game_struct.x == 3);
        }

        Debug_PlaybackStop(io);
    }
}

#define PRINT_FUNC_NAME(...) printf("%s...\n", __FILE_NAME__);

void
Test_LoopWhenMemoryExceeded()
{
    PRINT_FUNC_NAME();

    GameStruct game_struct{ 0 };
    auto       rp_struct = Debug_Make_RecordPlaybackFromMemory(Bytes(4));

    // Get into record mode.
    // NOTE(DW): Remember when we change state we also write the first block.
    Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, true);
    assert(rp_struct.game_mode == DEBUG_GAME_MODE_RECORDING);

    // Write to memory 6 times, causing the write location to wrap.
    {
        for (auto i = 0u; i < 4; ++i)
        {
            game_struct.x = i + 1;
            Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, false);
        }
        game_struct.x += 1;

        // Write last value and get into playback mode.
        // NOTE(DW): Remember when we change state we also read the first block.
        Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, true);
    }

    assert(rp_struct.game_mode == DEBUG_GAME_MODE_PLAYBACK);

    // We should now be reading from index 2.
    assert(game_struct.x == 2);

    Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, false);
    assert(game_struct.x == 3);
    Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, false);
    assert(game_struct.x == 4);
    Debug_HandleRecordOrPlaybackFromMemory(rp_struct, &game_struct, false);
    assert(game_struct.x == 5);
}

void
Test_DebugServices()
{
    Test_TimeBlockUsageExample();
    Test_RecordAndPlaybackFromFile();
    Test_RecordAndPlaybackFromMemory();
    Test_LoopWhenMemoryExceeded();
    printf("TEST DEBUG_SERVICES COMPLETE\n");
}
