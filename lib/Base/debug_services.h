#pragma once

#include "Base/dllexports.h"
#include "Base/platform/platform.h"
#include "Base/typedefs.h"
#if defined(_MSC_VER)
#include <SDL_rwops.h>
#else
#include <SDL2/SDL_rwops.h>
#endif
#include <array>
#include <atomic>
#include <cassert>
#include <limits.h>
#include <map>
#include <stdio.h>

#ifdef DEBUG_BUILD

//////////////////////////////////////////////////////////////////////////////

struct Debug_TimeBlockRecord
{
    // char const* file_name; // Note: Stored in Debug_TimeBlockStore.
    char const* func_name;
    uint32      line_number;

    uint64 cycles;
    uint64 count;
};

public_struct Debug_TimeBlockStore
{
    using FileToRecordsMap = std::map<char const*, std::array<Debug_TimeBlockRecord, 128>>;

    FileToRecordsMap file_name_to_records_map;
};

public_struct TimeBlock
{
    Debug_TimeBlockRecord* record;
    std::atomic<uint64>    start_cycles;

    TimeBlock(Debug_TimeBlockStore * store, char const* file_name, char const* func_name, int line_number, int counter);
    ~TimeBlock();
};

//////////////////////////////////////////////////////////////////////////////

public_var extern Debug_TimeBlockStore* global_debug_time_block_store;

public_func void
Debug_InitGlobalDebugServices();

public_func void
Debug_FreeGlobalDebugServices();


public_func
    Debug_TimeBlockRecord*
    Debug_GetTimeBlockRecord(Debug_TimeBlockStore* store, char const* file_name, int counter);

public_func void
Debug_PrintTimeBlockRecords(Debug_TimeBlockStore* store);

public_func void
Debug_ResetTimeBlockRecords(Debug_TimeBlockStore* store);

// We might pack count and cycles together at some point so its useful to have an interface.
inline uint64
Debug_TimeBlockRecord_Count(Debug_TimeBlockRecord const& record)
{
    return record.count;
}

// We might pack count and cycles together at some point so its useful to have an interface.
inline uint64
Debug_TimeBlockRecord_Cycles(Debug_TimeBlockRecord const& record)
{
    return record.cycles;
}

//////////////////////////////////////////////////////////////////////////////

#define __TIME_BLOCK(store, file_name, func_name, line, counter) TimeBlock _time_block##line(store, file_name, func_name, line, counter)
#define _TIME_BLOCK(store, file_name, func_name, line, counter) __TIME_BLOCK(store, file_name, func_name, line, counter)

#define DEBUG_INIT_GLOBAL_DEBUG_SERVICES(...) Debug_InitGlobalDebugServices()
#define DEBUG_FREE_GLOBAL_DEBUG_SERVICES(...) Debug_FreeGlobalDebugServices()

#define DEBUG_PRINT_TIME_BLOCK_RECORDS(...) Debug_PrintTimeBlockRecords(global_debug_time_block_store)
#define DEBUG_RESET_TIME_BLOCK_RECORDS(...) Debug_ResetTimeBlockRecords(global_debug_time_block_store)

#define TIME_BLOCK _TIME_BLOCK(global_debug_time_block_store, __FILE__, __PRETTY_FUNCTION__, __LINE__, __COUNTER__)

//////////////////////////////////////////////////////////////////////////////

public_func SDL_RWops*
            Debug_RecordingStartFromFile(char const* file_name);

public_func SDL_RWops*
            Debug_RecordingStartFromMemory(void* memory, uint64 size);

public_func SDL_RWops*
            Debug_PlaybackStartFromFile(char const* file_name);

public_func SDL_RWops*
            Debug_PlaybackStartFromMemory(void* memory, uint64 size);

public_func size_t
Debug_RecordingStop(SDL_RWops* io);

public_func void
Debug_PlaybackStop(SDL_RWops* io);

template <typename Tp>
size_t
Debug_RecordingWrite(SDL_RWops* io, Tp* data)
{
    auto bytes = SDL_RWwrite(io, (void*)data, sizeof(Tp), 1);
    return bytes;
}

template <typename Tp>
void
Debug_PlaybackRead(SDL_RWops* io, Tp* data, int64 end = LONG_MAX)
{
    auto sizex = sizeof(Tp);
    auto bytes = SDL_RWread(io, (void*)data, sizex, 1);

    auto pos = SDL_RWtell(io);
    // NOTE(DW): pos > end only applies when reading from memory.
    if (bytes == 0 || pos > end)
    {
        // Go to beginning of file.
        SDL_RWseek(io, 0, RW_SEEK_SET);

        // Attempt to read the data again as it would have just failed or been invalid.
        bytes = SDL_RWread(io, (void*)data, sizex, 1);
        assert(bytes);
    }
}

using DebugGameMode = uint8;

constexpr DebugGameMode const DEBUG_GAME_MODE_NORMAL    = 0;
constexpr DebugGameMode const DEBUG_GAME_MODE_RECORDING = 1;
constexpr DebugGameMode const DEBUG_GAME_MODE_PLAYBACK  = 2;

using DebugRecordMode = uint8;

constexpr DebugRecordMode const DEBUG_RECORD_MODE_STOP_AT_END = 0;
constexpr DebugRecordMode const DEBUG_RECORD_MODE_WRAP        = 1;

struct Debug_RecordPlaybackFromMemory
{
    DebugGameMode   game_mode;
    DebugRecordMode record_mode;

    SDL_RWops* io;
    void*      memory;
    size_t     size;
    size_t     end;
    bool       memory_wrapped;
};

inline Debug_RecordPlaybackFromMemory
Debug_Make_RecordPlaybackFromMemory(uint64          memory_size,
                                    DebugRecordMode record_mode = DEBUG_RECORD_MODE_WRAP)
{
    Debug_RecordPlaybackFromMemory rp_struct;
    rp_struct.game_mode      = DEBUG_GAME_MODE_NORMAL;
    rp_struct.record_mode    = record_mode;
    rp_struct.size           = memory_size;
    rp_struct.memory         = Platform_AllocateVirtualMemory(rp_struct.size);
    rp_struct.end            = 0;
    rp_struct.memory_wrapped = false;
    rp_struct.io             = nullptr;
    return rp_struct;
}

inline void
Debug_Free_RecordPlaybackFromMemory(Debug_RecordPlaybackFromMemory& rp_struct)
{
    if (rp_struct.io)
    {
        SDL_RWclose(rp_struct.io);
    }

    Platform_FreeVirtualMemory(rp_struct.memory, rp_struct.size);
}

template <typename Tp>
void
Debug_HandleRecordOrPlaybackFromMemory(Debug_RecordPlaybackFromMemory& state,
                                       Tp*                             window,
                                       bool                            button_pressed)
{
    auto& game_mode   = state.game_mode;
    auto  record_mode = state.record_mode;
    auto& end         = state.end;
    auto& io          = state.io;
    auto* memory      = state.memory;
    auto  memory_size = state.size;

    switch (game_mode)
    {
    case DEBUG_GAME_MODE_NORMAL: {
        if (button_pressed)
        {
            printf("Recording started...\n");

            game_mode = DEBUG_GAME_MODE_RECORDING;
            io        = Debug_RecordingStartFromMemory(memory, memory_size);
            assert(io);

            // Note(DW): Remember that we must capture the current frame as external
            // code will see that the state has changed and therefore might expect the
            // first block of data to have been written to memory.
            Debug_RecordingWrite(io, window);
        }
        break;
    }
    case DEBUG_GAME_MODE_RECORDING: {
        auto bytes = Debug_RecordingWrite(io, window);
        if (bytes == 0 && record_mode == DEBUG_RECORD_MODE_WRAP)
        {
            printf("Memory wrapped\n");

            state.memory_wrapped = true;

            // Go to beginning of file ...
            SDL_RWseek(io, 0, RW_SEEK_SET);

            // ... and attempt write again since it would have just failed.
            bytes = Debug_RecordingWrite(io, window);
            assert(bytes);
        }

        if (button_pressed)
        {
            printf("Playback started...\n");

            game_mode = DEBUG_GAME_MODE_PLAYBACK;
            end       = Debug_RecordingStop(io);
            io        = Debug_PlaybackStartFromMemory(memory, memory_size);
            assert(io);

            // NOTE(DW): If we wrap, then we seek to the record stop position + 1 as this is the oldest data.
            // Then, because we have wrapped and have used the entire buffer, end becomes the memory_size as this
            // informs Debug_PlaybackRead where it should read up to.
            if (state.memory_wrapped)
            {
                // Go to the oldest data.
                SDL_RWseek(io, end, RW_SEEK_SET);
                end = memory_size;
            }

            // Note(DW): Remember that we must load the current frame as external
            // code will see that the state is now in PLAYBACK mode and will expect
            // that the first block of data has been loaded.
            Debug_PlaybackRead(io, window, end);
        }
        break;
    }
    case DEBUG_GAME_MODE_PLAYBACK: {
        Debug_PlaybackRead(io, window, end);

        if (button_pressed)
        {
            printf("Playback stopped\n");

            game_mode = DEBUG_GAME_MODE_NORMAL;
            Debug_PlaybackStop(io);
            io = nullptr;
        }
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////////

#else // RELEASE BUILD

//////////////////////////////////////////////////////////////////////////////

#define DEBUG_INIT_DEBUG_SERVICES(...)
#define DEBUG_FREE_DEBUG_SERVICES(...)

#define DEBUG_PRINT_TIME_BLOCK_RECORDS(...)
#define DEBUG_RESET_TIME_BLOCK_RECORDS(...)

#define TIME_BLOCK(...)

//////////////////////////////////////////////////////////////////////////////

#endif
