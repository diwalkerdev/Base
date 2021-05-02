#pragma once

#include "containers/array.h"
#include "dllexports.h"
#include "highresclock.h"
#include "typedefs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>
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
constexpr Event const EVENT_RENDER                = 2;
constexpr Event const EVENT_SIMULATION            = 3;
constexpr Event const EVENT_WINDOW_RESIZED        = 4;
constexpr Event const EVENT_WINDOW_MOVED          = 5;
constexpr Event const EVENT_TOGGLE_RECORDPLAYBACK = 6;
constexpr Event const EVENT_SIZE           = 7; // Must be last.

using EventTable = std::array<Event, EVENT_SIZE>;

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

#define compile_u8(name, value) constexpr uint8 const(name) = (value)
#define compile_float(name, value) constexpr float const(name) = (value)
compile_u8(KEY_BINDING_LCTRL, 0x01);
compile_u8(KEY_BINDING_RCTRL, 0x02);
compile_u8(KEY_BINDING_LALT, 0x04);
compile_u8(KEY_BINDING_RALT, 0x08);
compile_u8(KEY_BINDING_LGUI, 0x10);
compile_u8(KEY_BINDING_RGUI, 0x20);

public_struct KeyBinding
{
    SDL_Scancode scan_code;
    KeyState     state;
    Event        event;
    uint8        flags;
};


public_struct EventManager
{
    HighResCounter clk;
    uint64         dcount;
    EventTable     event_table{ 0 };

    Array<EventTimer, EVENT_MANAGER_NUM_TIMERS> timers;
    Array<KeyBinding, EVENT_MANAGER_NUM_KEYS>   key_bindings;
};


template <size_t Nm>
void
EventManager_AddKeyBinding(Array<KeyBinding, Nm>& key_bindings, SDL_Scancode key, Event event, uint8 flags = 0)
{
    key_bindings.reserve(1);

    key_bindings.back() = { .scan_code = key,
                            .state     = KEY_RELEASED,
                            .event     = event,
                            .flags     = flags };
}


public_func void
EventManager_AddKeyBinding(EventManager& event_manager, SDL_Scancode key, Event event, uint8 flags = 0);


template <size_t Nm>
KeyBinding*
EventManager_FindKeyBinding(Array<KeyBinding, Nm>& key_bindings, char key)
{
    for (auto& binding : key_bindings)
    {
        if (binding.scan_code == key)
        {
            return &binding;
        }
    }
    return nullptr;
}


public_func KeyBinding*
            EventManager_FindKeyBinding(EventManager& event_manager, char key);


public_func void
EventManager_AddTimer(EventManager& event_manager, float fps, Event event);


public_func Event
Event_ConvertFromSDLEvent(SDL_Event& event);


public_func void
Event_UpdateKeyState(KeyState& state, int current_value);

template <size_t Nm>
void
Event_PollSDLKeyboardEvents(Array<KeyBinding, Nm>& key_bindings, EventTable& event_table)
{
    uint8 const* keyboard_state = SDL_GetKeyboardState(NULL);

    auto const lalt  = keyboard_state[SDL_SCANCODE_LALT];
    auto const ralt  = keyboard_state[SDL_SCANCODE_RALT];
    auto const lctrl = keyboard_state[SDL_SCANCODE_LCTRL];
    auto const rctrl = keyboard_state[SDL_SCANCODE_RCTRL];
    auto const lgui  = keyboard_state[SDL_SCANCODE_LGUI];
    auto const rgui  = keyboard_state[SDL_SCANCODE_RGUI];

    for (auto& binding : key_bindings)
    {
        auto const key_state = keyboard_state[binding.scan_code];

        if ((binding.flags & KEY_BINDING_LCTRL && !lctrl) || (binding.flags & KEY_BINDING_RCTRL && !rctrl))
        {
            continue;
        }

        if ((binding.flags & KEY_BINDING_LALT && !lalt) || (binding.flags & KEY_BINDING_RALT && !ralt))
        {
            continue;
        }

        if ((binding.flags & KEY_BINDING_LGUI && !lgui) || (binding.flags & KEY_BINDING_RGUI && !rgui))
        {
            continue;
        }

        Event_UpdateKeyState(binding.state, key_state);

        if (binding.event != EVENT_NO_EVENT && binding.state == KEY_PRESSED)
        {
            event_table[binding.event] += 1;
        }
    }
}


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
