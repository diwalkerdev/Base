#include "Base/debug_services.h"
#include "Base/platform/sdl/sdl_events.h"
#include "Base/platform/sdl/sdl_window.h"
#include "GeometricAlgebra/geometric_algebra.h"
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


struct Components
{
    Int           state_idx;
    Int           input_idx;
    Int           position_idx;
    Int           projection_idx;
    Int           velocity_idx;
    Int           bounding_box_idx;
    Array<Int, 8> texture_idx;
};


enum class ComponentId
{
    State,
    Input,
    Position,
    Projection,
    Velocity,
    BoundingBox,
    Texture,
};


int
BitToIndex(int val, int num_bits = 32)
{
    for (int index = 0; index < num_bits; ++index)
    {
        auto bit = 1 << index;
        if (val & bit)
        {
            return index;
        }
    }
    return -1;
}


using UpdateStateFunction = void (*)(Components const&);

struct StateComponent
{
    UpdateStateFunction UpdateState;

    Array<Int, 8>   action_texture_map;
    Array<float, 8> action_timer_map;

    UInt movement;

    UByte action;
    float action_timer;

    Array<UByte, 10> queue;
};


enum class InputActions
{
    None,
    Attack,
};


struct InputComponent
{
    Vec movement;
    // We assume each animation is about 600ms or 38 sim steps.
    // We only look at the current input (1st element) and then only need two frames
    // to determine a chain of 3.
    Array<InputActions, 64> queue;
};


struct PositionComponent
{
    float x, y;
};


struct ProjectionComponent
{
    float x, y;
};


struct VelocityComponent
{
    Vec v;
};


struct BoundingBoxComponent
{
    Vec   offset;
    Vec   size;
    UByte r, g, b, a;
};


struct TextureComponent
{
    SDL_Texture* texture;
    Int          n_sprites;
    Int          sprite_w;
    Int          sprite_h;
    Int          scale;
    Vec          offset;
    Vec          stride;

    bool  animate;
    float frame_t;
    float anim_t;
    int   frame;
};


struct EntityComponentSystem
{
    Array<StateComponent, 16>       states;
    Array<PositionComponent, 16>    positions;
    Array<ProjectionComponent, 16>  projections;
    Array<VelocityComponent, 16>    velocities;
    Array<InputComponent, 16>       inputs;
    Array<BoundingBoxComponent, 16> bounding_boxes;
    Array<TextureComponent, 32>     textures;
};


enum PlayerAction
{
    None     = 0x00,
    Attack_1 = 0x01,
    Attack_2 = 0x02,
    Attack_3 = 0x04,
};

struct Player
{
    Components components;
};


struct GameStruct
{
    Window                window;
    SDL_EventQueue        event_q;
    SDL_EventKeyFilter    filter;
    SDL_EventKeyFilter    player_input_filter;
    bool                  running;
    EntityComponentSystem ecs;
    Player                player;
} game_struct;


//////////////////////////////////////////////////////////////////////////////


UInt
Component_Reserve(EntityComponentSystem& ecs, ComponentId id)
{
    switch (id)
    {
        case ComponentId::State:
        {
            ecs.states.reserve(1);
            return ecs.states.size() - 1;
        }
        case ComponentId::Input:
        {
            ecs.inputs.reserve(1);
            return ecs.inputs.size() - 1;
        }
        case ComponentId::Position:
        {
            ecs.positions.reserve(1);
            return ecs.positions.size() - 1;
        }
        case ComponentId::Projection:
        {
            ecs.projections.reserve(1);
            return ecs.projections.size() - 1;
        }
        case ComponentId::Velocity:
        {
            ecs.velocities.reserve(1);
            return ecs.velocities.size() - 1;
        }
        case ComponentId::BoundingBox:
        {
            ecs.bounding_boxes.reserve(1);
            return ecs.bounding_boxes.size() - 1;
        }
        case ComponentId::Texture:
        {
            ecs.textures.reserve(1);
            return ecs.textures.size() - 1;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////


void
System_UpdateMovement(EntityComponentSystem& ecs,
                      Components const&      components,
                      float                  dt,
                      bool                   update_projection = false)
{
    StateComponent*      state      = nullptr;
    InputComponent*      input      = nullptr;
    PositionComponent*   position   = nullptr;
    ProjectionComponent* projection = nullptr;
    VelocityComponent*   velocity   = nullptr;

    InputComponent    default_input { 0, 0 };
    VelocityComponent default_velocity { 0, 0 };

    if (components.state_idx >= 0)
    {
        state = &ecs.states[components.state_idx];
    }

    if (components.input_idx >= 0)
    {
        input = &ecs.inputs[components.input_idx];
    }
    else
    {
        input = &default_input;
    }

    // Must have a position.
    assert(components.position_idx >= 0);
    {
        position = &ecs.positions[components.position_idx];
    }

    bool is_moving = false;
    if (components.velocity_idx >= 0)
    {
        velocity = &ecs.velocities[components.velocity_idx];
        if (Vec_Magnitude(input->movement) > 0.5)
        {
            velocity->v = input->movement;
        }

        float const friction = 0.1;
        if (Vec_Magnitude(velocity->v) > 0.01)
        {
            is_moving = true;

            auto opposite = Vec_Normalise(velocity->v) * -1.0f;
            opposite *= friction;

            velocity->v += opposite;
        }
        else
        {
            velocity->v.x = 0;
            velocity->v.y = 0;
        }
    }
    else
    {
        velocity = &default_velocity;
    }

    if (state)
    {
        state->movement = is_moving ? 1 : 0;
    }

    float x_dot = position->x + ((300.0f * velocity->v.x) * dt);
    float y_dot = position->y + ((300.0f * velocity->v.y) * dt);

    if (update_projection && components.projection_idx >= 0)
    {
        projection    = &ecs.projections[components.projection_idx];
        projection->x = x_dot;
        projection->y = y_dot;
    }
    else
    {
        position->x = x_dot;
        position->y = y_dot;
    }
}


void
System_AnimateTextures(EntityComponentSystem& ecs, float sim_t)
{
    TIME_BLOCK;

    for (auto& state : ecs.states)
    {
        if (state.action != 0)
        {
            if (state.action_timer >= 0)
            {
                state.action_timer -= sim_t;
            }
            if (state.action_timer < 0)
            {
                SDL_Log("ending action\n");
                state.action = 0;
                state.queue.pop_front(0);
            }
        }
    }

    for (auto& texture : ecs.textures)
    {
        if (!texture.animate)
        {
            texture.frame  = 0;
            texture.anim_t = 0;
            continue;
        }

        texture.anim_t += sim_t;
        while (texture.anim_t > texture.frame_t)
        {
            texture.anim_t -= texture.frame_t;
            texture.frame += 1;
            if (texture.frame >= texture.n_sprites)
            {
                texture.frame = 0;
            }
        }
    }
}


void
System_UpdateStates()
{
    // TODO(DW): need a way of iterating over the entities in the game.
    // for (auto& state : game_struct.ecs.states)
    // {
    //     if (state.UpdateState)
    //     {
    //     }
    // }
    auto& state = game_struct.ecs.states[game_struct.player.components.state_idx];
    if (state.UpdateState)
    {
        state.UpdateState(game_struct.player.components);
    }
}


void
System_UpdateInputQueues()
{
    for (auto& input : game_struct.ecs.inputs)
    {
        std::shift_right(input.queue.begin(), input.queue.end(), 1);
        input.queue[0] = InputActions::None;
    }
}


//////////////////////////////////////////////////////////////////////////////

void
Player_UpdateState(Components const& components);

void
Player_Init(Player& player, EntityComponentSystem& ecs)
{
    player.components                = { -1 };
    player.components.state_idx      = Component_Reserve(ecs, ComponentId::State);
    player.components.input_idx      = Component_Reserve(ecs, ComponentId::Input);
    player.components.position_idx   = Component_Reserve(ecs, ComponentId::Position);
    player.components.projection_idx = Component_Reserve(ecs, ComponentId::Projection);
    player.components.velocity_idx   = Component_Reserve(ecs, ComponentId::Velocity);
    int texture_idx_1                = Component_Reserve(ecs, ComponentId::Texture);
    int texture_idx_2                = Component_Reserve(ecs, ComponentId::Texture);
    int texture_idx_3                = Component_Reserve(ecs, ComponentId::Texture);
    int texture_idx_4                = Component_Reserve(ecs, ComponentId::Texture);
    int texture_idx_5                = Component_Reserve(ecs, ComponentId::Texture);
    player.components.texture_idx.push_back(texture_idx_1);
    player.components.texture_idx.push_back(texture_idx_2);
    player.components.texture_idx.push_back(texture_idx_3);
    player.components.texture_idx.push_back(texture_idx_4);
    player.components.texture_idx.push_back(texture_idx_5);

    StateComponent& state         = ecs.states[player.components.position_idx];
    state.UpdateState             = &Player_UpdateState;
    state.movement                = 0;
    state.action_texture_map[0]   = texture_idx_3;
    state.action_texture_map[1]   = texture_idx_4;
    state.action_texture_map[2]   = texture_idx_5;
    state.action_texture_map.last = 4;
    state.action_timer_map[0]     = 0.1f * 6;
    state.action_timer_map[1]     = 0.1f * 6;
    state.action_timer_map[2]     = 0.1f * 5;
    state.action_timer_map.last   = 4;

    InputComponent& input = ecs.inputs[player.components.input_idx];
    input.movement        = { 0, 0 };
    input.queue.reserve_all();

    PositionComponent& position = ecs.positions[player.components.position_idx];
    position.x                  = 0;
    position.y                  = 0;

    TextureComponent& texture_idle = ecs.textures[player.components.texture_idx[0]];
    texture_idle.n_sprites         = 1;
    texture_idle.sprite_w          = 128;
    texture_idle.sprite_h          = 64;
    texture_idle.scale             = 3;
    texture_idle.stride            = { 0, 64 };
    texture_idle.frame_t           = 0.1;
    texture_idle.anim_t            = 0;
    texture_idle.frame             = 0;
    texture_idle.texture           = IMG_LoadTexture(game_struct.window.renderer.renderer,
                                           "Hero/Sprites/Idle.png");
    assert(texture_idle.texture != nullptr);

    TextureComponent& texture_run = ecs.textures[player.components.texture_idx[1]];
    texture_run.n_sprites         = 6;
    texture_run.sprite_w          = 128;
    texture_run.sprite_h          = 64;
    texture_run.scale             = 3;
    texture_run.stride            = { 0, 64 };
    texture_run.frame_t           = 0.1;
    texture_run.anim_t            = 0;
    texture_run.frame             = 0;
    texture_run.texture           = IMG_LoadTexture(game_struct.window.renderer.renderer,
                                          "Hero/Sprites/Run & Hop.png");
    assert(texture_run.texture != nullptr);

    // TODO(DW): Order - needs reference to texture_run.
    BoundingBoxComponent& bb = ecs.bounding_boxes[player.components.bounding_box_idx];
    bb.offset                = { 50.0f * texture_run.scale, 35.0f * texture_run.scale };
    bb.size                  = { 8.0f * texture_run.scale, 12.0f * texture_run.scale };

    TextureComponent& texture_attack_1 = ecs.textures[player.components.texture_idx[2]];
    texture_attack_1.n_sprites         = 6;
    texture_attack_1.sprite_w          = 128;
    texture_attack_1.sprite_h          = 64;
    texture_attack_1.scale             = 3;
    texture_attack_1.offset            = { 0, 0 };
    texture_attack_1.stride            = { 0, 64 };
    texture_attack_1.frame_t           = 0.1;
    texture_attack_1.anim_t            = 0;
    texture_attack_1.frame             = 0;
    texture_attack_1.texture           = IMG_LoadTexture(game_struct.window.renderer.renderer,
                                               "Hero/Sprites/Chain Attack.png");
    assert(texture_attack_1.texture != nullptr);

    TextureComponent& texture_attack_2 = ecs.textures[player.components.texture_idx[3]];
    texture_attack_2.n_sprites         = 6;
    texture_attack_2.sprite_w          = 128;
    texture_attack_2.sprite_h          = 64;
    texture_attack_2.scale             = 3;
    texture_attack_2.offset            = { 0, 6 * 64 };
    texture_attack_2.stride            = { 0, 64 };
    // Note(DW): The queue gets shifted
    texture_attack_2.frame_t = 0.1;
    texture_attack_2.anim_t  = 0;
    texture_attack_2.frame   = 0;
    texture_attack_2.texture = texture_attack_1.texture;
    assert(texture_attack_2.texture != nullptr);

    TextureComponent& texture_attack_3 = ecs.textures[player.components.texture_idx[4]];
    texture_attack_3.n_sprites         = 6;
    texture_attack_3.sprite_w          = 128;
    texture_attack_3.sprite_h          = 64;
    texture_attack_3.scale             = 3;
    texture_attack_3.offset            = { 0, 12 * 64 };
    texture_attack_3.stride            = { 0, 64 };
    texture_attack_3.frame_t           = 0.1;
    texture_attack_3.anim_t            = 0;
    texture_attack_3.frame             = 0;
    texture_attack_3.texture           = texture_attack_1.texture;
    assert(texture_attack_3.texture != nullptr);
}


void
Player_OnUpdateInput(Array<SDL_Event, SDL_EVENTQUEUESIZE>& events, void* _)
{
    static float a = 0.0f, s = 0.0f, d = 0.0f, w = 0.0f;

    auto actions = InputActions::None;

    for (auto& event : events)
    {
        switch (event.type)
        {
            case SDL_KEYUP:
            {
                SDL_Log("key up\n");
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_A:
                    {
                        SDL_Log("A\n");
                        a = 0.0f;
                        break;
                    }
                    case SDL_SCANCODE_S:
                    {
                        s = 0.0f;
                        break;
                    }
                    case SDL_SCANCODE_D:
                    {
                        SDL_Log("D\n");
                        d = 0.0f;
                        break;
                    }
                    case SDL_SCANCODE_W:
                    {
                        w = 0.0f;
                        break;
                    }
                    case SDL_SCANCODE_K:
                    {
                        // NOTE(DW): Actions get turned off automatically based on a timer.
                        SDL_Log("Release k attack\n");
                        break;
                    }
                    default:
                    {
                        SDL_Log("Unknown scancode: %d\n", event.key.keysym.scancode);
                    }
                }
                break;
            }
            case SDL_KEYDOWN:
            {
                SDL_Log("key down\n");
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_A:
                    {
                        // SDL_Log("A\n");
                        a = 1.0f;
                        break;
                    }
                    case SDL_SCANCODE_S:
                    {
                        s = 1.0f;
                        break;
                    }
                    case SDL_SCANCODE_D:
                    {
                        // SDL_Log("D\n");
                        d = 1.0f;
                        break;
                    }
                    case SDL_SCANCODE_W:
                    {
                        w = 1.0f;
                        break;
                    }
                    case SDL_SCANCODE_K:
                    {
                        SDL_Log("Input k attack\n");
                        // actions |= PLAYER_ACTION_ATTACK;
                        actions = InputActions::Attack;
                        break;
                    }
                    default:
                    {
                        SDL_Log("Unknown scancode: %d\n", event.key.keysym.scancode);
                    }
                }
                break;
            }
        }
    }
    auto& player = game_struct.player;
    auto& input  = game_struct.ecs.inputs[player.components.input_idx];

    auto lr          = d - a;
    auto ud          = s - w;
    input.movement.x = lr;
    input.movement.y = ud;

    if (Vec_Magnitude(input.movement) > 0.1)
    {
        input.movement = Vec_Normalise(input.movement);
    }
    else
    {
        input.movement = { 0, 0, 0 };
    }

    // Note(DW): The queue gets shifted every sim loop in UpdateInputQueues.
    if (actions != InputActions::None)
    {
        input.queue[0] = actions;
    }
}


void
Player_UpdateState(Components const& components)
{
    // TODO: This is very specific to the player.
    assert(components.input_idx >= 0);
    assert(components.state_idx >= 0);

    auto& input = game_struct.ecs.inputs[components.input_idx];
    auto& state = game_struct.ecs.states[components.state_idx];

    auto region_1 = input.queue.begin();
    auto region_2 = input.queue.begin() + 1;
    auto region_3 = input.queue.begin() + 38;
    auto region_4 = input.queue.begin() + 64;

    {
        auto i1 = std::count(region_1, region_2, InputActions::Attack);
        auto i2 = std::count(region_2, region_3, InputActions::Attack);
        auto i3 = std::count(region_3, region_4, InputActions::Attack);

        if (i1 == 1 && i2 == 1 && i3 == 1)
        {
            if (std::count(state.queue.begin(), state.queue.end(), PlayerAction::Attack_3) == 0)
            {
                SDL_Log("Attack 3");
                state.queue.push_back(PlayerAction::Attack_3);
            }
        }
        else if (i1 == 1 && i2 == 1)
        {
            if (std::count(state.queue.begin(), state.queue.end(), PlayerAction::Attack_2) == 0)
            {
                SDL_Log("Attack 2");
                state.queue.push_back(PlayerAction::Attack_2);
            }
        }
        else if (i1 == 1)
        {
            if (std::count(state.queue.begin(), state.queue.end(), PlayerAction::Attack_1) == 0)
            {
                SDL_Log("Attack 1");
                state.queue.push_back(PlayerAction::Attack_1);
            }
        }
    }


    if ((state.action == PlayerAction::None) && (state.queue.size() > 0))
    {
        // Note(DW): The item gets popped when the timer expires.
        state.action       = state.queue[0];
        Int index          = BitToIndex(state.action);
        state.action_timer = state.action_timer_map[index];
    }
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
        System_UpdateMovement(game_struct.ecs,
                              game_struct.player.components,
                              SIM_PERIOD);

        System_UpdateStates();
        System_UpdateInputQueues();

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

    // TODO(DW): This render system is very specific to the player.
    EntityComponentSystem& ecs            = game_struct.ecs;
    Components const&      components     = game_struct.player.components;
    StateComponent*        state          = nullptr;
    PositionComponent*     position       = nullptr;
    ProjectionComponent*   projection     = nullptr;
    BoundingBoxComponent*  bounding_box   = nullptr;
    TextureComponent*      active_texture = nullptr;

    System_UpdateMovement(ecs,
                          components,
                          remainder_t,
                          /*update_projections*/ true);

    SDL_WindowClear(game_struct.window, 100, 100, 100, 255);

    if (components.state_idx >= 0)
    {
        state = &ecs.states[components.state_idx];
    }
    assert(components.position_idx >= 0);
    {
        position = &ecs.positions[components.position_idx];
    }
    if (components.projection_idx >= 0)
    {
        projection = &ecs.projections[components.projection_idx];
    }
    if (components.bounding_box_idx >= 0)
    {
        bounding_box = &ecs.bounding_boxes[components.bounding_box_idx];
    }


    if (state && state->movement)
    {
        active_texture = &ecs.textures[components.texture_idx[1]];
    }
    else
    {
        active_texture = &ecs.textures[components.texture_idx[0]];
    }

    if (state && (state->action != PlayerAction::None))
    {
        Int index      = BitToIndex(state->action);
        Int texture_id = state->action_texture_map[index];
        active_texture = &ecs.textures[texture_id];
    }

    for (auto tindx : components.texture_idx)
    {
        ecs.textures[tindx].animate = false;
    }
    if (active_texture)
    {
        active_texture->animate = true;
    }

    float alpha = remainder_t / RENDER_PERIOD;
    float x0    = position->x;
    float y0    = position->y;
    float x1 = 0, y1 = 0;

    // SDL_Log("Alpha %f\n", alpha);

    if (projection)
    {
        x1 = projection->x;
        y1 = projection->y;
    }
    else
    {
        alpha = 1.0f;
    }

    // static float dx = 0;

    auto xdot = (x0 * alpha) + (x1 * (1.0f - alpha));
    auto ydot = (y0 * alpha) + (y1 * (1.0f - alpha));

    // SDL_Log("dx: %f\n", xdot - dx);
    // dx = xdot;

    UByte r = 255, g = 255, b = 255, a = 255;
    if (bounding_box)
    {
        r = bounding_box->r;
        g = bounding_box->g;
        b = bounding_box->b;
        a = bounding_box->a;

        SDL_FRect rect { xdot + bounding_box->offset.x,
                         ydot + bounding_box->offset.y,
                         bounding_box->size.x,
                         bounding_box->size.y };

        SDL_SetRenderDrawColor(game_struct.window.renderer.renderer,
                               r,
                               g,
                               b,
                               a);

        SDL_RenderDrawRectF(game_struct.window.renderer.renderer, &rect);
    }

    if (active_texture)
    {
        auto     offset = (active_texture->stride * active_texture->frame) + active_texture->offset;
        SDL_Rect src { int(offset.x + 0.5),
                       int(offset.y + 0.5),
                       active_texture->sprite_w,
                       active_texture->sprite_h };
        SDL_Rect dst { Cast(int, position->x + 0.5),
                       Cast(int, position->y + 0.5),
                       active_texture->sprite_w * active_texture->scale,
                       active_texture->sprite_h * active_texture->scale };

        SDL_RenderCopy(game_struct.window.renderer.renderer,
                       active_texture->texture,
                       &src,
                       &dst);
    }

    SDL_RendererPresent(game_struct.window);
}


//////////////////////////////////////////////////////////////////////////////


void
Setup_CapturePlayerInput(SDL_EventKeyFilter& filter)
{
    filter._callback = &Player_OnUpdateInput;
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
        SDL_Log("Window event: %d\n", event.window.event);
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
    // Animate textures after Update and Render. This is because you want
    // the order to be update states, render those states, and then progress the states.
    // If you animate between the Update and Render, then the state changes, and then
    // the state is immediately progressed, causes the animation to glitch slightly.
    System_AnimateTextures(game_struct.ecs, dt);

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

    running = true;
    SDL_EventQueueInit(event_q, 32);
    Setup_CaptureEscapeKey(filter, &running);
    Setup_CapturePlayerInput(game_struct.player_input_filter);
    Player_Init(game_struct.player, game_struct.ecs);

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
