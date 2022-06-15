#include "Base/platform/sdl/sdl_mem_recorder.h"
#if defined(_MSC_VER)
#include <SDL_rwops.h>
#else
#include <SDL2/SDL_rwops.h>
#endif

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