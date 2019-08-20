// Copyright 2019 David Butler <croepha@gmail.com>


#include <easy/profiler.h>
#include <SDL2/SDL.h>

#include "common.hpp"


extern bool _debug_should_quit;
extern bool _debug_ignoring_events;
SDL_Window* sdl_window;

void ds_platform_init() {
    
    
    SDL_Init(SDL_INIT_VIDEO);
    
    char* window_title = "ASDFASDF";
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    
    assert(!*SDL_GetError());
    
    sdl_window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    
    auto gl_context = SDL_GL_CreateContext(sdl_window);
    assert(gl_context);
    assert(!*SDL_GetError());
    
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    printf("SDL_ERROR: %s\n", SDL_GetError());
    assert(!*SDL_GetError());
    
    printf("SDL_ERROR: %s\n", SDL_GetError());
    assert(!*SDL_GetError());
    
    SDL_GL_SetSwapInterval(1); // set vsync
    //SDL_GL_SetSwapInterval(0); // disable vsync
    
    SDL_EnableScreenSaver();
    
    assert(!*SDL_GetError());
    
    
}


void ds_platform_frame() {
    EASY_FUNCTION();
    
    SDL_GL_SwapWindow(sdl_window);
    
    SDL_Event _sde;
    while (SDL_PollEvent(&_sde)) {
        switch (_sde.type) {
            case SDL_QUIT: {
                _debug_should_quit = 1;
                
            } break;
            case SDL_KEYDOWN: {
                if (_sde.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
            } break;
            case SDL_WINDOWEVENT: {
                switch(_sde.window.event) {
                    case SDL_WINDOWEVENT_FOCUS_GAINED: {
                        _debug_ignoring_events = 0;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    } break;
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        _debug_ignoring_events = 1;
                    } break;
                }
            } break;
        };
    }
}
