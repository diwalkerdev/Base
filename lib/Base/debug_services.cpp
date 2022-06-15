#include "Base/debug_services.h"
#include "Base/platform/platform.h"
#include <SDL2/SDL.h>
#include <cassert>


Debug_TimeBlockStore* global_debug_time_block_store { nullptr };

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
    static float const CYCLES_PER_SEC = Platform_GetPerformanceFrequency();
    float              min_pcent;
    float              max_pcent;

    for (auto const& item : store->file_name_to_records_map)
    {
        SDL_Log("%s\n", item.first);
        SDL_Log("%6s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
                "Line",
                "Duration",
                "Count",
                "Cycles",
                "Avg",
                "Min",
                "Min%",
                "Max",
                "Max%",
                "Func");

        for (auto i = 0u; i < item.second.size(); ++i)
        {
            auto const& record = item.second[i];

            if (record.count > 0)
            {
                min_pcent = (float(record._min - record._avg) / record._avg) * 100;
                max_pcent = (float(record._max - record._avg) / record._avg) * 100;

                SDL_Log("%6d %10f %10lu %10lu %10f %10f %10.1f %10f %10.1f %10s\n",
                        record.line_number,
                        record.cycles / CYCLES_PER_SEC,
                        record.count,
                        record.cycles,
                        record._avg / CYCLES_PER_SEC,
                        record._min / CYCLES_PER_SEC,
                        min_pcent,
                        record._max / CYCLES_PER_SEC,
                        max_pcent,
                        record.func_name);
            }
        }

        SDL_Log("\n");
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

    start_cycles = Platform_GetPerformanceCounter();
}

TimeBlock::~TimeBlock()
{
    Long cycles = (Platform_GetPerformanceCounter() - start_cycles);
    record->cycles += cycles;
    record->count += 1;

    record->_min = std::min(record->_min, cycles);
    record->_max = std::max(record->_max, cycles);
    if (record->count > 1)
    {
        // NOTE(DW): We purposely don't round here as we don't
        // care about fractions of a cycle.
        record->_avg += cycles;
        record->_avg /= 2;
    }
    else
    {
        record->_avg = cycles;
    }
}

//////////////////////////////////////////////////////////////////////////////
