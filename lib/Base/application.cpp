#include "application.h"


void
EventManager_AddKeyBinding(EventManager& event_manager, SDL_Scancode key, Event event, uint8 flags)
{
    EventManager_AddKeyBinding(event_manager.key_bindings, key, event, flags);
}


KeyBinding*
EventManager_FindKeyBinding(EventManager& event_manager, char key)
{
    auto* binding = EventManager_FindKeyBinding(event_manager.key_bindings, key);
    assert(binding != nullptr);
    return binding;
}


void
EventManager_AddTimer(EventManager& event_manager, float fps, Event event)
{
    auto& timers = event_manager.timers;
    Array_Reserve(timers);

    timers.back() = { .counter = 0,
                      .count   = Cast(uint64, (SDL_GetPerformanceFrequency() / fps)),
                      .event   = event };
}


Event
Event_ConvertFromSDLEvent(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        return EVENT_QUIT;
    case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
            return EVENT_WINDOW_RESIZED;
        case SDL_WINDOWEVENT_MOVED:
            return EVENT_WINDOW_MOVED;
        }
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            return EVENT_MOUSE_LEFT_DOWN;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            return EVENT_MOUSE_RIGHT_DOWN;
        }
        else
        {
            return EVENT_MOUSE_UNKNOWN_DOWN;
        }
    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            return EVENT_MOUSE_LEFT_UP;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            return EVENT_MOUSE_RIGHT_UP;
        }
        else
        {
            return EVENT_MOUSE_UNKNOWN_UP;
        }
    case SDL_MOUSEMOTION:
        return EVENT_MOUSE_MOVED;
    }
    return EVENT_NO_EVENT;
}

void
Event_UpdateKeyState(KeyState& state, int current_value)
{
    if (current_value)
    {
        if (state == KEY_RELEASED)
        {
            state += 1;
        }
        else if (state == KEY_PRESSED)
        {
            state += 1;
        }
        else if (state == KEY_HELD)
        {
            // do nothing
        }
    }
    else
    {
        if (state == KEY_RELEASED)
        {
            // do nothing
        }
        else if (state == KEY_PRESSED)
        {
            state = KEY_RELEASED;
        }
        else if (state == KEY_HELD)
        {
            state = KEY_RELEASED;
        }
    }
}


void
Event_PollSDLEvents(EventManager& event_manager)
{
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event) != 0)
    {
        Event event = Event_ConvertFromSDLEvent(sdl_event);

        if (event == EVENT_MOUSE_MOVED)
        {
            event_manager.mouse_state.x = sdl_event.motion.x;
            event_manager.mouse_state.y = sdl_event.motion.y;
        }

        if (event == EVENT_MOUSE_LEFT_DOWN || event == EVENT_MOUSE_LEFT_UP)
        {
            auto& mouse_state = event_manager.mouse_state.left_mouse;

            mouse_state.clicks = sdl_event.button.clicks;

            if (event == EVENT_MOUSE_LEFT_DOWN)
            {
                mouse_state.state = MOUSE_BUTTON_PRESSED;
            }
            else if (event == EVENT_MOUSE_LEFT_UP)
            {
                mouse_state.state = MOUSE_BUTTON_RELEASED;
            }
        }

        if (event == EVENT_MOUSE_RIGHT_DOWN || event == EVENT_MOUSE_RIGHT_UP)
        {
            auto& mouse_state = event_manager.mouse_state.right_mouse;

            mouse_state.clicks = sdl_event.button.clicks;

            if (event == EVENT_MOUSE_RIGHT_DOWN)
            {
                mouse_state.state = MOUSE_BUTTON_PRESSED;
            }
            else if (event == EVENT_MOUSE_RIGHT_UP)
            {
                mouse_state.state = MOUSE_BUTTON_RELEASED;
            }
        }

        event_manager.event_table[event] = 1;
    }
}


void
Event_PollTimeEvents(EventManager& event_manager)
{
    auto dcount          = ElapsedCount(event_manager.clk);
    event_manager.dcount = dcount;
    // printf("dcount: %lu\n", dcount);

    for (auto& timer : event_manager.timers)
    {
        timer.counter += dcount;

        // The while loop is required, because when we come out of a record/playback loop,
        // we will get a big dcount, for reasons I don't know. However, the impact is if
        // we have an if statement, then timer.counter will be greater than timer.count for
        // a number if loops so we get several simulation events in a row. Having the while
        // loop eats up the excess time, and this is indicated to higher level code by
        // incrementing the counter more than once.
        while (timer.counter > timer.count)
        {
            // printf("#\n");
            timer.counter -= timer.count;
            event_manager.event_table[timer.event] += 1;
        }
    }
}

void
Event_Poll(EventManager& event_manager)
{
    Event_PollSDLKeyboardEvents(event_manager.key_bindings, event_manager.event_table);
    Event_PollSDLEvents(event_manager);
    Event_PollTimeEvents(event_manager);
}


int
Event_Query(EventManager& event_manager, int event)
{
    return Event_Query(event_manager.event_table, event);
}


int
Event_Query(EventTable& table, int event)
{
    return table[event];
}


int
Event_QueryAndReset(EventManager& event_manager, int event, int clear_value)
{
    return Event_QueryAndReset(event_manager.event_table, event, clear_value);
}


int
Event_QueryAndReset(EventTable& table, int event, int clear_value)
{
    auto result  = table[event];
    table[event] = clear_value;
    return result;
}

void
Window_Init(Window& the_window, int screen_width, int screen_height, bool use_hw_acceleration)
{
    SDL_Window* window;
    {
        window = SDL_CreateWindow("Window Resizing",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  screen_width,
                                  screen_height,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        if (!window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create window: %s.\n",
                         SDL_GetError());
            the_window.error = SYS_CREATE_WINDOW_ERROR;
            return;
        }
    }

    SDL_Renderer* renderer;
    {
        auto renderer_mode = use_hw_acceleration
            ? (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
            : SDL_RENDERER_SOFTWARE;

        // -1 to initialize the first driver supporting the requested flags.
        renderer = SDL_CreateRenderer(window, -1, renderer_mode);

        if (!renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create renderer: %s.\n",
                         SDL_GetError());
            the_window.error = SYS_CREATE_RENDERER_ERROR;
            return;
        }
    }

    the_window.window   = window;
    the_window.renderer = renderer;
    SDL_GetWindowSize(the_window.window, &the_window.w, &the_window.h);
    SDL_GetWindowPosition(the_window.window, &the_window.x, &the_window.y);
}


void
Window_Free(Window& window)
{
    SDL_DestroyWindow(window.window);
    SDL_DestroyRenderer(window.renderer);
}


void
Window_Clear(Window& window)
{
    SDL_SetRenderTarget(window.renderer, nullptr);
    SDL_SetRenderDrawColor(window.renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(window.renderer);
}


void
Window_PresentRenderer(Window& window)
{
    SDL_RenderPresent(window.renderer);
}


void
Window_HandleEvents(Window& window, EventTable& events)
{
    if (Event_QueryAndReset(events, EVENT_WINDOW_RESIZED, 0))
    {
        SDL_GetWindowSize(window.window, &window.w, &window.h);
        printf("Window_HandleEvents: window resized %d %d\n", window.w, window.h);
        // events[EVENT_RENDER] += 1;
    }
    if (Event_QueryAndReset(events, EVENT_WINDOW_MOVED, 0))
    {
        SDL_GetWindowPosition(window.window, &window.x, &window.y);
        printf("Window_HandleEvents: window moved %d %d\n", window.x, window.y);
        // events[EVENT_RENDER] += 1;
    }
}


void
Window_LoadState(Window* window)
{
    SDL_SetWindowSize(window->window, window->w, window->h);
    SDL_SetWindowPosition(window->window, window->x, window->y);
}
