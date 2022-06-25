#include "Base/debug_services.h"
#include "Base/platform/sdl/sdl_events.h"
#include "Base/platform/sdl/sdl_window.h"
#include "GeometricAlgebra/geometric_algebra.h"
#include "Layout/panel.h"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cassert>
#include <stdio.h>


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

constexpr float const RENDER_HZ     = 30.0f;
constexpr float const RENDER_PERIOD = 1.0f / RENDER_HZ;
constexpr float const SIM_HZ        = 60.0f;
constexpr float const SIM_PERIOD    = 1.0f / SIM_HZ;


//////////////////////////////////////////////////////////////////////////////


struct GameStruct
{
    Window             window;
    SDL_EventQueue     event_q;
    SDL_EventKeyFilter filter;
    SDL_EventKeyFilter player_input_filter;
    bool               running;
    Panel              panel;
} game_struct;


//////////////////////////////////////////////////////////////////////////////


#define N_ITEMS 64

static Node        nodes[N_ITEMS];
static Layout      layouts[N_ITEMS];
static int         seen[N_ITEMS];
static char const* ids[N_ITEMS] = { 0 };

void
CenteredMainMenu_Init(Panel* panel)
{
    // Add placeholders for widgets.
    Size min_size = { 100, 20 };
    Size max_size = { 200, 50 };

    // Init the panel.
    // A windowed panel can be created using Panel_InitWindowed.
    // Requires Panel_SetPanelLayoutWindowed (see below).
    //
    int root_index = Panel_Init(panel,
                                "panel",
                                &nodes[0],
                                &layouts[0],
                                &ids[0],
                                &seen[0],
                                N_ITEMS);

    // Set how the panel resizes. Options are:
    // Panel_SetPanelLayoutFixed
    // Panel_SetPanelLayoutMinimal
    // Panel_SetPanelLayoutWindowed, for use with Panel_InitWindowed.
    //
    Panel_SetPanelLayoutExpand(panel, 1.0, 1.0);

    // Set the default layout of the panel.
    // TODO(DW): Panel_SetLayoutVertical/Horizontal?
    Panel_SetLayout(panel,
                    root_index,
                    LayoutType_Horizontal,
                    SizePolicy_Expand,
                    SizePolicy_Expand,
                    1,
                    1);

    // Add children to the panel:
    //

    Panel_AddSpan(panel, root_index, 1, 1);

    {
        int v_index = Panel_AddLayout(panel,
                                      root_index,
                                      LayoutType_Vertical,
                                      SizePolicy_Expand,
                                      SizePolicy_Expand,
                                      1,
                                      1);

        // int v_index = Panel_Add(panel,
        //                         root_index,
        //                         1,
        //                         1);


        // Provide some application specific overrides to the central vertical layout.
        //
        Layout* v_layout    = &panel->layouts[v_index];
        v_layout->h.padding = 20;

        Panel_AddSpan(panel, v_index, 1.0, 1.0);

        Panel_AddWidget(panel,
                        "button#1",
                        v_index,
                        min_size,
                        max_size,
                        1.0,
                        1.0);

        Panel_AddWidget(panel,
                        "button#2",
                        v_index,
                        min_size,
                        max_size,
                        1.0,
                        1.0);

        Panel_AddWidget(panel,
                        "button#3",
                        v_index,
                        min_size,
                        max_size,
                        1.0,
                        1.0);

        Panel_AddSpan(panel, v_index, 1, 1);
    }

    Panel_AddSpan(panel, root_index, 1, 1);
}


void
CenteredMainMenu_Draw()
{
    Panel*    panel = &game_struct.panel;
    Rectangle r;

    SDL_SetRenderDrawColor(game_struct.window.renderer.renderer,
                           255,
                           255,
                           255,
                           255);
    r = Panel_WidgetRect(panel, "panel");
    // GuiPanelEx(panel, "Panel"); // Draw border here if panel is windowed.
    SDL_Log("Panel w:%f h:%f\n", r.width, r.height);

    SDL_RenderFillRectF(game_struct.window.renderer.renderer,
                        (SDL_FRect*)&r);

    SDL_SetRenderDrawColor(game_struct.window.renderer.renderer,
                           255,
                           0,
                           0,
                           255);
    r = Panel_WidgetRect(panel, "button#1");
    // GuiButton(r, "Start Game");
    SDL_RenderFillRectF(game_struct.window.renderer.renderer,
                        (SDL_FRect*)&r);

    SDL_SetRenderDrawColor(game_struct.window.renderer.renderer,
                           0,
                           255,
                           0,
                           255);
    r = Panel_WidgetRect(panel, "button#2");
    // GuiButton(r, "Options");
    SDL_RenderFillRectF(game_struct.window.renderer.renderer,
                        (SDL_FRect*)&r);

    SDL_SetRenderDrawColor(game_struct.window.renderer.renderer,
                           0,
                           0,
                           255,
                           255);
    r = Panel_WidgetRect(panel, "button#3");
    // GuiButton(r, "Quit Game");
    SDL_RenderFillRectF(game_struct.window.renderer.renderer,
                        (SDL_FRect*)&r);
}


//////////////////////////////////////////////////////////////////////////////


float
Update(float dt)
{
    static float acc = 0;
    acc += dt;

    if (acc < SIM_PERIOD)
    {
        return acc;
    }

    TIME_BLOCK;

    while (acc >= SIM_PERIOD)
    {
        acc -= SIM_PERIOD;
    }

    return acc;
}


void
Render(float dt, float remainder_t)
{
    static float acc       = 0;
    static int   fps_count = 0;
    static float fps_timer = 0.0f;

    acc += dt;
    fps_timer += dt;

    if (acc < RENDER_PERIOD)
    {
        return;
    }

    while (acc > RENDER_PERIOD)
    {
        acc -= RENDER_PERIOD;
    }


    fps_count += 1;
    if (fps_timer > 1.0)
    {
        SDL_Log("Rendered %d fps", fps_count);
        fps_timer = 0;
        fps_count = 0;
    }


    TIME_BLOCK;

    SDL_WindowClear(game_struct.window, 100, 100, 100, 255);
    Panel_Update(&game_struct.panel);
    CenteredMainMenu_Draw();

    SDL_RendererPresent(game_struct.window);
}


//////////////////////////////////////////////////////////////////////////////


void
Setup_CapturePlayerInput(SDL_EventKeyFilter& filter)
{
    filter._callback = nullptr;
    filter._key_filter.push_back(SDL_SCANCODE_A);
    filter._key_filter.push_back(SDL_SCANCODE_S);
    filter._key_filter.push_back(SDL_SCANCODE_D);
    filter._key_filter.push_back(SDL_SCANCODE_W);
    filter._key_filter.push_back(SDL_SCANCODE_K);
}


void
App_OnKeyEscape(Array<SDL_Event, SDL_EVENTQUEUESIZE>& events, void* userdata)
{
    SDL_Log("App On Escape\n");
    bool* running = Cast(bool*, userdata);
    *running      = false;
}


void
Setup_CaptureEscapeKey(SDL_EventKeyFilter& filter, bool* running)
{
    filter._callback = &App_OnKeyEscape;
    filter._userdata = Cast(void*, running);
    filter._key_filter.push_back(SDL_SCANCODE_ESCAPE);
}


void
App_OnQuit()
{
    Window& window = game_struct.window;
    // SDL_EventQueue&     event_q = game_struct.event_q;
    // SDL_EventKeyFilter& filter  = game_struct.filter;
    // bool&               running = game_struct.running;

    SDL_Log("MainLoop ended\n");
    SDL_WindowFree(window);
    SDL_Quit();

    Debug_PrintTimeBlockRecords(global_debug_time_block_store);
    Debug_FreeGlobalDebugServices();
}


void
App_OnWindowEvent(Array<SDL_Event, SDL_EVENTQUEUESIZE>& window_events)
{
    TIME_BLOCK;

    for (auto& event : window_events)
    {
        switch (event.window.event)
        {
            case SDL_WINDOWEVENT_RESIZED:
            {
                int width  = event.window.data1;
                int height = event.window.data2;
                SDL_Log("Window Resized Event. w: %d h: %d", width, height);

                Layouts_WindowResized(width, height);
            }
        }
    }
}


void
App_MainLoop()
{
    TIME_BLOCK;

    static_assert(SIM_HZ > RENDER_HZ);
    static ULong prev = Platform_GetPerformanceCounter();
    ULong        now  = Platform_GetPerformanceCounter();
    float        dt   = Platform_ToSeconds(now - prev);
    // Window&             window  = game_struct.window;
    SDL_EventQueue& event_q = game_struct.event_q;
    bool&           running = game_struct.running;

    SDL_EventsProcess(event_q);

    if (event_q.quit)
    {
        running = false;
    }

    App_OnWindowEvent(event_q.window_events);
    SDL_EventKeyFilterCheckEvent(game_struct.filter,
                                 event_q.keyboard_events);
    SDL_EventKeyFilterCheckEvent(game_struct.player_input_filter,
                                 event_q.keyboard_events);

    SDL_EventQueueClear(event_q);

    float remainder = Update(dt);
    Render(dt, remainder);

    prev = now;


    if (!running)
    {
#ifdef __EMSCRIPTEN__
        SDL_Log("Ending main loop");
        emscripten_cancel_main_loop();
        quit();
#endif
    }
}


int
main()
{
    Debug_InitGlobalDebugServices();
    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    assert(result == 0);

    int IMG_FLAGS = IMG_INIT_PNG;
    result        = IMG_Init(IMG_FLAGS) & IMG_FLAGS;
    assert(result != 0);

    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Hello Application\n");

    Window&             window  = game_struct.window;
    SDL_EventQueue&     event_q = game_struct.event_q;
    SDL_EventKeyFilter& filter  = game_struct.filter;
    bool&               running = game_struct.running;

    auto window_starting_pos = SDL_Rect { SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          800,
                                          600 };

    window = SDL_WindowMake("Game",
                            window_starting_pos,
                            (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE),
                            (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

    assert(window.error == WindowError::NO_ERROR);

    Layouts_WindowResized(800, 600);

    running = true;
    SDL_EventQueueInit(event_q, 32);
    Setup_CaptureEscapeKey(filter, &running);
    Setup_CapturePlayerInput(game_struct.player_input_filter);

    global_style = StyleSheet_MakeDefaultStyle();
    CenteredMainMenu_Init(&game_struct.panel);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, -1);
#else
    while (running)
    {
        App_MainLoop();
        SDL_Delay(1);
    }
    App_OnQuit();
#endif
    return 0;
}
