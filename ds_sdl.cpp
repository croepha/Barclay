// Copyright 2019 David Butler <croepha@gmail.com>

#include <easy/profiler.h>


#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "common.hpp"


extern EGLDisplay egl_display;
extern EGLSurface egl_surface;
void egl_init1();
void egl_init2();
void egl_init3(void* native_window);
void egl_frame();


extern bool _debug_should_quit;
extern bool _debug_ignoring_events;

SDL_Window* sdl_window;




void ds_platform_init() {
    
    egl_init1();
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    egl_init2();
    
    SDL_Init(SDL_INIT_VIDEO);
    
    char* window_title = "ASDFASDF";
    
    sdl_window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    SDL_SysWMinfo window_info;
    SDL_VERSION(&window_info.version);
    SDL_GetWindowWMInfo(sdl_window,&window_info);
    assert(window_info.subsystem == SDL_SYSWM_X11);
    
    egl_init3((void*)window_info.info.x11.window);
    
    
    
    
    SDL_EnableScreenSaver();
    
    assert(!*SDL_GetError());
    
    
}


void ds_platform_frame() {
    EASY_FUNCTION();
    
    egl_frame();
    
    
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
