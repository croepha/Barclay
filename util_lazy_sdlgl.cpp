// Copyright 2019 David Butler <croepha@gmail.com>

#if !defined(NO_DEFINE_IMPLEMENTATION)
#pragma clang diagnostic ignored "-Wunused-macros"
#endif

#define SDL_stdinc_h_
#include <SDL2/SDL_scancode.h>
#define SDL_BUTTON(X)       (1 << ((X)-1))
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5
#define SDL_BUTTON_LMASK    SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK    SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK    SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK   SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK   SDL_BUTTON(SDL_BUTTON_X2)
#undef SDL_stdinc_h_
#include "util_types.hpp"

void sdlgl_init(char*title, int initial_width, int initial_height);
void sdlgl_frame();

struct __sdlgl_SingletonSpace {
    u8 _pvt[1024];
    const u8  keyboard_state_old[256];
    const u8* keyboard_state;
    u32 mouse_state;
    V2<s32> mouse_pos;
    V2<s32> mouse_pos_last;
    V2<s32> mouse_wheel_delta;
    bool should_quit;
    bool has_focus;
    u16 frame_time_delta_ms;
    u16 swap_time_last_ms;
    u64 frame_time_last_ms;
    
};

extern __sdlgl_SingletonSpace sdlgl;

#define sdlgl_KEYDOWN(m_key) (sdlgl.keyboard_state[SDL_SCANCODE_ ## m_key])
#define sdlgl_KEY(m_key)     ({ assert(SDL_SCANCODE_ ## m_key < 256); sdlgl.keyboard_state[SDL_SCANCODE_ ## m_key] && !sdlgl.keyboard_state_old[SDL_SCANCODE_ ## m_key];})
#define sdlgl_MOUSEDOWN(m_key) (!!(sdlgl.mouse_state & SDL_BUTTON(SDL_BUTTON_ ## m_key)))

u32 get_ticks_ms();

#if defined(DEFINE_SINGLETONS)

__sdlgl_SingletonSpace sdlgl = {};

#endif
#if !defined(NO_DEFINE_IMPLEMENTATION)

#include <easy/profiler.h>

#include <SDL2/SDL.h>

#define NO_DEFINE_IMPLEMENTATION
#include "util_gl.cpp"
#undef NO_DEFINE_IMPLEMENTATION

struct Private {
    SDL_Window* window;
    SDL_GLContext gl_context;
};
//static_assert(sizeof(Singleton) == sizeof(__sdlgl_SingletonSpace));
static_assert(sizeof(Private) <= sizeof(__sdlgl_SingletonSpace));

#define USE_PRIVATE auto&_pvt=*(Private*)&sdlgl._pvt

void sdlgl_init(char*title, int initial_width, int initial_height) { USE_PRIVATE;
    
    auto r9 = SDL_Init(SDL_INIT_VIDEO);
    assert(!r9);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    
    _pvt.window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        initial_width, initial_height, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    assert(_pvt.window);
    
    _pvt.gl_context = SDL_GL_CreateContext(_pvt.window );
    assert(_pvt.gl_context);
    
    SDL_GL_SetSwapInterval(1); // set vsync
    //SDL_GL_SetSwapInterval(0); // disable vsync
    SDL_EnableScreenSaver();
    
    _gl_init();
    
    sdlgl.keyboard_state = SDL_GetKeyboardState(0);
    
    sdlgl.frame_time_last_ms = SDL_GetTicks();
    
    
}


void sdlgl_frame() {  USE_PRIVATE;
    
    EASY_FUNCTION();
    
    
    EASY_BLOCK("SDL_GL_SwapWindow");
    
    
    {
        auto _ms1 = SDL_GetTicks();
        SDL_GL_SwapWindow(_pvt.window);
        auto _ms2 = SDL_GetTicks();
        sdlgl.swap_time_last_ms = (u16)(_ms2 - _ms1);
    }
    
    
    EASY_END_BLOCK;
    
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    
    assert(glGetError() == GL_NO_ERROR);
    
    
    {
        auto frame_time_ms = SDL_GetTicks();
        sdlgl.frame_time_delta_ms = (u16)(frame_time_ms - sdlgl.frame_time_last_ms);
        sdlgl.frame_time_last_ms = frame_time_ms;
        //    printf("frame_time_delta: %u\n", frame_time_delta);
    }
    
    sdlgl.mouse_wheel_delta = {0,0};
    
    memcpy((void*)sdlgl.keyboard_state_old, sdlgl.keyboard_state, 256);
    
    SDL_Event _sde;
    while (SDL_PollEvent(&_sde)) {
        switch (_sde.type) {
            case SDL_QUIT: {
                sdlgl.should_quit = 1;
            } break;
            case SDL_MOUSEWHEEL: {
                s32 _mul = 1;
                if (_sde.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    _mul = -1;
                } 
                sdlgl.mouse_wheel_delta.x += _sde.wheel.x * _mul;
                sdlgl.mouse_wheel_delta.y += _sde.wheel.y * _mul;
            } break;
            case SDL_KEYDOWN: {
                if (_sde.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
            } break;
            case SDL_WINDOWEVENT: {
                switch(_sde.window.event) {
                    case SDL_WINDOWEVENT_FOCUS_GAINED: {
                        sdlgl.has_focus = 1;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    } break;
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        sdlgl.has_focus = 0;
                    } break;
                }
            } break;
        };
    }
    
    sdlgl.mouse_pos_last = sdlgl.mouse_pos;
    
    sdlgl.mouse_state = SDL_GetMouseState(
        &sdlgl.mouse_pos.x, 
        &sdlgl.mouse_pos.y);
    
    sdlgl.keyboard_state = SDL_GetKeyboardState(0);
    
    
}

u32 get_ticks_ms() {
    return SDL_GetTicks();
}



#endif
