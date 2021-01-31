#ifndef LIBRARY_H
#define LIBRARY_H
#include <SDL.h>
#include <cassert>
#include <filesystem>

struct LibraryLoader {
    std::filesystem::path           library_path;
    std::filesystem::file_time_type last_write_time;
    void*                           library_ptr = nullptr;

    void set_library_name(const char* library_name)
    {
        library_path = std::filesystem::path(library_name);
        SDL_assert(std::filesystem::exists(library_path));
    }

    bool should_reload(int* error_code)
    {
        // SDL_Log("LibraryLoader::should_reload");
        if (!std::filesystem::exists(library_path))
        {
            SDL_Log("%s does not exist.", library_path.c_str());
            *error_code = 1;
            return false;
        }

        // Everything should be ok at this point.
        //
        *error_code    = 0;
        auto file_time = std::filesystem::last_write_time(library_path);
        if (file_time == last_write_time)
        {
            return false;
        }

        last_write_time = file_time;
        return true;
    }

    int reload()
    {
        // SDL_Log("LibraryLoader::reload");
        const char* library_name = library_path.filename().c_str();

        if (library_ptr != nullptr)
        {
            SDL_Log("Unloading %s", library_name);
            SDL_UnloadObject(&library_ptr);
            library_ptr = nullptr;
        }

        // SDL_Log("Loading %s", library_name);
        library_ptr = SDL_LoadObject(library_name);
        if (library_ptr == nullptr)
        {
            SDL_Log("Could not load %s: %s.\n", library_name, SDL_GetError());
            return 1;
        }

        return 0;
    }
};

#endif // LIBRARY_H