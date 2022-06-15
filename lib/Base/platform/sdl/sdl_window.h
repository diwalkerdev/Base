#pragma once

#include "Base/dllexports.h"
#include "Base/typedefs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>

#define SCREEN_HEIGHT(renderer) (int32)(*renderer.screen_h)
#define TO_EUCLID(renderer, x) SCREEN_HEIGHT(renderer) - (x)


enum class WindowError
{
    NO_ERROR              = 0,
    CREATE_WINDOW_ERROR   = 1,
    CREATE_RENDERER_ERROR = 2,
};


public_struct Renderer
{
    SDL_Renderer* renderer;
    int32*        screen_h;
};


public_struct Window
{
    SDL_Window* window;
    Renderer    renderer;

    int32 w, h;
    int32 x, y;

    WindowError error;
};


public_func Window
SDL_WindowMake(cstring   title,
               SDL_Rect& screen_pos,
               UInt      window_flags,
               UInt      renderer_flags);


public_func void
SDL_WindowFree(Window& window);


public_func void
SDL_WindowClear(Window& window, UByte r, UByte g, UByte b, UByte a);


public_func void
SDL_RendererPresent(Window& window);
