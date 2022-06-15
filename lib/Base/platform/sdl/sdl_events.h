#pragma once
#include "Base/containers/array.h"
#include "Base/typedefs.h"
#include <SDL2/SDL.h>

#define SDL_EVENTQUEUESIZE 32

// enum class EventId
// {
//     Quit,
//     Step,
//     Window,
//     Input,
// };


// struct Event
// {
//     SDL_Event sdl_event;

// bool
// operator==(EventId id)
// {
//     return this->id == id;
// }
// };


//////////////////////////////////////////////////////////////////////////////

struct SDL_EventQueue
{
    bool step;
    bool quit;

    Array<SDL_Event, SDL_EVENTQUEUESIZE> keyboard_events;
    Array<SDL_Event, SDL_EVENTQUEUESIZE> window_events;

    SDL_TimerID step_timer;
};


UInt
SDL_OnTimerCallback(UInt interval, void* param)
{
    SDL_Event     event;
    SDL_UserEvent userevent;

    /* In this example, our callback pushes an SDL_USEREVENT event
    into the queue, and causes our callback to be called again at the
    same interval: */

    userevent.type  = SDL_USEREVENT;
    userevent.code  = 1;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return (interval);
}


void
SDL_EventQueueInit(SDL_EventQueue& q, UInt step_interval)
{
    q.step_timer = SDL_AddTimer(step_interval, SDL_OnTimerCallback, nullptr);
}


void
SDL_EventQueueClear(SDL_EventQueue& q)
{
    q.step = false;
    q.quit = false;
    q.window_events.clear();
    q.keyboard_events.clear();
}

//////////////////////////////////////////////////////////////////////////////

using SDL_EventKeyCallback = void (*)(Array<SDL_Event, SDL_EVENTQUEUESIZE>&, void*);

struct SDL_EventKeyFilter
{
    SDL_EventKeyCallback                 _callback;
    void*                                _userdata;
    Array<SDL_Scancode, 64>              _key_filter;
    Array<SDL_Event, SDL_EVENTQUEUESIZE> _current_events;
};


void
SDL_EventKeyFilterCheckEvent(SDL_EventKeyFilter&                   filter,
                             Array<SDL_Event, SDL_EVENTQUEUESIZE>& keyboard_events)
{
    filter._current_events.clear();

    for (auto& event : keyboard_events)
    {
        for (auto& code : filter._key_filter)
        {
            if (event.key.keysym.scancode == code)
            {
                filter._current_events.push_back(event);
            }
        }
    }

    if (filter._current_events.size() > 0)
    {
        filter._callback(filter._current_events, filter._userdata);
    }
}

//////////////////////////////////////////////////////////////////////////////

void
SDL_EventsProcess(SDL_EventQueue& q)
{
    SDL_Event sdl_event;

    bool is_okay = false;
    while (SDL_PollEvent(&sdl_event) != 0)
    {
        switch (sdl_event.type)
        {
            case SDL_QUIT:
            {
                q.quit = true;
                break;
            }
            case SDL_WINDOWEVENT:
            {
                is_okay = q.window_events.push_back({ sdl_event });
                assert(is_okay);
                break;
            }
            case SDL_KEYDOWN:
            {
                is_okay = q.keyboard_events.push_back({ sdl_event });
                assert(is_okay);
                break;
            }
            case SDL_KEYUP:
            {
                is_okay = q.keyboard_events.push_back({ sdl_event });
                assert(is_okay);
                break;
            }
            case SDL_USEREVENT:
            {
                if (sdl_event.user.code == 1)
                {
                    q.step = true;
                }
                else
                {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Unknown User Event Id: %d\n",
                                sdl_event.user.code);
                }
                break;
            }
        }
        is_okay = false;
    }
}
