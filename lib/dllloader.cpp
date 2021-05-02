#include "dllloader.h"

#if defined(_MSC_VER)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <cassert>


SharedLibrary
Make_SharedLibrary(char const* library_path)
{
    namespace fs = std::filesystem;

    SharedLibrary lib;
    lib.path = fs::path(library_path);
    lib.ptr  = nullptr;

    assert(fs::exists(lib.path));

    return lib;
}


int
SharedLibraryReload(SharedLibrary& lib, bool force)
{
    bool library_modified = SharedLibraryShouldReload(lib);

    if (!library_modified && !force)
    {
        return 0;
    }

    if (lib.ptr != nullptr)
    {
        SDL_UnloadObject(&lib.ptr);
        lib.ptr = nullptr;
    }

#if defined(_MSC_VER)
#include <SDL.h>
    // TODO: need to provide platform versions of these functions.
    lib.ptr = nullptr;
#else
#include <SDL2/SDL.h>
    char const* library_name = lib.path.filename().c_str();
    printf("Loading library %s\n", library_name);
    lib.ptr = SDL_LoadObject(library_name.c_str());
#endif

    assert(lib.ptr != nullptr);

    return 0;
}


bool
SharedLibraryShouldReload(SharedLibrary& lib)
{
    namespace fs = std::filesystem;

    assert(fs::exists(lib.path));

    auto file_time = fs::last_write_time(lib.path);
    if (file_time == lib.write_time)
    {
        return false;
    }

    lib.write_time = file_time;
    return true;
}
