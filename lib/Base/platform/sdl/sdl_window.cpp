#include "Base/platform/sdl/sdl_window.h"

Window
SDL_WindowMake(cstring   title,
               SDL_Rect& screen_pos,
               UInt      window_flags,
               UInt      renderer_flags)
{
    Window      the_window;
    SDL_Window* window;
    {
        window = SDL_CreateWindow(title,
                                  screen_pos.x,
                                  screen_pos.y,
                                  screen_pos.w,
                                  screen_pos.h,
                                  window_flags);


        if (window == nullptr)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create window: %s.\n",
                         SDL_GetError());
            the_window.error = WindowError::CREATE_WINDOW_ERROR;
            return the_window;
        }
    }

    SDL_Renderer* renderer;
    {
        // Note(DW): -1 says to initialize the first driver supporting the requested flags.
        renderer = SDL_CreateRenderer(window, -1, renderer_flags);

        if (renderer == nullptr)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create renderer: %s.\n",
                         SDL_GetError());
            the_window.error = WindowError::CREATE_RENDERER_ERROR;
            return the_window;
        }
    }

    the_window.error             = WindowError::NO_ERROR;
    the_window.window            = window;
    the_window.renderer.renderer = renderer;
    the_window.renderer.screen_h = &the_window.h;
    SDL_GetWindowSize(the_window.window, &the_window.w, &the_window.h);
    SDL_GetWindowPosition(the_window.window, &the_window.x, &the_window.y);
    SDL_WindowClear(the_window, 255, 255, 255, 255);
    SDL_RendererPresent(the_window);
    return the_window;
}


void
SDL_WindowFree(Window& window)
{
    SDL_DestroyRenderer(window.renderer.renderer);
    SDL_DestroyWindow(window.window);
}


void
SDL_WindowClear(Window& window, UByte r, UByte g, UByte b, UByte a)
{
    SDL_SetRenderTarget(window.renderer.renderer, nullptr);
    SDL_SetRenderDrawColor(window.renderer.renderer, r, g, b, a);
    SDL_RenderClear(window.renderer.renderer);
}


void
SDL_RendererPresent(Window& window)
{
    SDL_RenderPresent(window.renderer.renderer);
}
