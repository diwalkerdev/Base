#include "debug_services.h"
#if defined(_MSC_VER)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <cassert>

Debug_TimeBlockStore* global_debug_time_block_store{ nullptr };

void
Debug_InitGlobalDebugServices()
{
    global_debug_time_block_store = new Debug_TimeBlockStore;
}

void
Debug_FreeGlobalDebugServices()
{
    delete global_debug_time_block_store;
}

Debug_TimeBlockRecord*
Debug_GetTimeBlockRecord(Debug_TimeBlockStore* store,
                         char const*           file_name,
                         int                   counter)
{
    assert(counter < 128);
    auto& records = store->file_name_to_records_map[file_name];
    return &records[counter];
}

void
Debug_PrintTimeBlockRecords(Debug_TimeBlockStore* store)
{
    float const CYCLES_PER_SEC = SDL_GetPerformanceFrequency();

    for (auto const& item : store->file_name_to_records_map)
    {
        printf("%s\n", item.first);

        for (auto i = 0u;
             i < item.second.size();
             ++i)
        {
            auto const& record = item.second[i];

            auto cycles = record.cycles;
            auto count  = record.count;

            if (count > 0)
            {
                printf("\t[%d] %45s  LN:%d  time:%f  Count:%lu  cycles:%lu\n",
                       i,
                       record.func_name,
                       record.line_number,
                       cycles / CYCLES_PER_SEC,
                       count,
                       cycles);
            }
        }
    }
}

void
Debug_ResetTimeBlockRecords(Debug_TimeBlockStore* store)
{
    for (auto& item : store->file_name_to_records_map)
    {
        auto& container = item.second;
        memset(&container[0], 0, sizeof(decltype(container[0])) * container.size());
    }
}

//////////////////////////////////////////////////////////////////////////////

TimeBlock::TimeBlock(Debug_TimeBlockStore* store, char const* file_name, char const* func_name, int line_number, int counter)
{
    record = Debug_GetTimeBlockRecord(store, file_name, counter);

    record->func_name   = func_name;
    record->line_number = line_number;

    start_cycles = SDL_GetPerformanceCounter();
}

TimeBlock::~TimeBlock()
{
    record->cycles += (SDL_GetPerformanceCounter() - start_cycles);
    record->count += 1;
}

//////////////////////////////////////////////////////////////////////////////

SDL_RWops*
Debug_RecordingStartFromFile(char const* file_name)
{
    return SDL_RWFromFile(file_name, "wb");
}

SDL_RWops*
Debug_RecordingStartFromMemory(void* memory, uint64 size)
{
    return SDL_RWFromMem(memory, size);
}

SDL_RWops*
Debug_PlaybackStartFromFile(char const* file_name)
{
    return SDL_RWFromFile(file_name, "rb");
}

SDL_RWops*
Debug_PlaybackStartFromMemory(void* memory, uint64 size)
{
    return SDL_RWFromMem(memory, size);
}

size_t
Debug_RecordingStop(SDL_RWops* io)
{
    auto pos = SDL_RWseek(io, 0, RW_SEEK_CUR);
    SDL_RWclose(io);
    return pos;
}

void
Debug_PlaybackStop(SDL_RWops* io)
{
    SDL_RWclose(io);
}
