#pragma once

#include "dllexports.h"
#include <filesystem>

public_struct SharedLibrary
{
    std::filesystem::path           path;
    std::filesystem::file_time_type write_time;
    void*                           ptr;
};

public_func SharedLibrary
Make_SharedLibrary(char const* library_path);


public_func int
SharedLibraryReload(SharedLibrary& info, bool force = false);


public_func bool
SharedLibraryShouldReload(SharedLibrary& lib);
