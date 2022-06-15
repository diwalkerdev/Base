#pragma once

#include "Base/dllexports.h"
#include "Base/platform/platform.h"
#include "Base/typedefs.h"
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
    Long   _avg = 0;
    Long   _min = UINT_MAX;
    Long   _max = 0;
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

public_var extern Debug_TimeBlockStore* global_debug_time_block_store;

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
