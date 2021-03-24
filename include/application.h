#pragma once

#include "containers/array.h"
#include "dllexports.h"
#include "highresclock.h"
#include "typedefs.h"
#include <SDL2/SDL.h>
#include <array>

#ifndef EVENT_MANAGER_NUM_TIMERS
#define EVENT_MANAGER_NUM_TIMERS 5
#endif
#ifndef EVENT_MANAGER_NUM_KEYS
#define EVENT_MANAGER_NUM_KEYS 5
#endif

using SystemError = uint8;

constexpr SystemError const SYS_NO_ERROR              = 0;
constexpr SystemError const SYS_CREATE_WINDOW_ERROR   = 1;
constexpr SystemError const SYS_CREATE_RENDERER_ERROR = 2;

// TODO: Event system needs to be easily extendable.
using Event = uint8;

constexpr Event const EVENT_NO_EVENT       = 0;
constexpr Event const EVENT_QUIT           = 1;
constexpr Event const EVENT_RENDER         = 2;
constexpr Event const EVENT_SIMULATION     = 3;
constexpr Event const EVENT_WINDOW_RESIZED = 4;
constexpr Event const EVENT_WINDOW_MOVED   = 5;
constexpr Event const EVENT_SIZE           = 6; // Must be last.

using EventTable = std::array<Event, EVENT_SIZE>;

constexpr int SCANCODE_TABLE[] = {
    SDL_SCANCODE_A,
    SDL_SCANCODE_B,
    SDL_SCANCODE_C,
    SDL_SCANCODE_D,
    SDL_SCANCODE_E,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_I,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_M,
    SDL_SCANCODE_N,
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_R,
    SDL_SCANCODE_S,
    SDL_SCANCODE_T,
    SDL_SCANCODE_U,
    SDL_SCANCODE_V,
    SDL_SCANCODE_W,
    SDL_SCANCODE_X,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_Z
};

using KeyState = int8;

constexpr KeyState const KEY_RELEASED = 0;
constexpr KeyState const KEY_PRESSED  = 1;
constexpr KeyState const KEY_HELD     = 2;

public_struct EventTimer
{
    uint64 counter;
    uint64 count;
    Event  event;
};

public_struct KeyBinding
{
    char     ch;
    KeyState state;
};

public_struct EventManager
{
    HighResCounter clk;
    uint64         dcount;
    EventTable     event_table{ 0 };

    Array<EventTimer, EVENT_MANAGER_NUM_TIMERS> timers;
    Array<KeyBinding, EVENT_MANAGER_NUM_KEYS>   key_bindings;
};

public_func EventManager
Make_EventManager(float render_fps, float sim_fps);

public_func Event
Event_ConvertFromSDLScancode(uint8 const* scan_state);

public_func Event
Event_ConvertFromSDLEvent(SDL_Event& event);

public_func void
Event_UpdateKeyState(KeyState& state, int current_value);

public_func void
Event_PollSDLEvents(EventManager& event_manager);

public_func void
Event_PollTimeEvents(EventManager& event_manager);

public_func void
Event_Poll(EventManager& event_manager);

public_func int
Event_QueryAndReset(EventTable& table, int event, int clear_value);


public_struct Window
{
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    int32         w = 0, h = 0;
    int32         x = 0, y = 0;
    SystemError   error = SYS_NO_ERROR;
};

public_func Window
Make_Window(int screen_width, int screen_height, bool hw_acceleration);

public_func void
Free_Window(Window& window);

public_func void
Window_Clear(Window& window);

public_func void
Window_PresentRenderer(Window& window);

public_func void
Window_HandleEvents(Window& window, EventTable& events);

public_func void
Window_LoadState(Window* window);
