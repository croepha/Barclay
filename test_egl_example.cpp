// Copyright 2019 David Butler <croepha@gmail.com>


#include <stdlib.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GL/gl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "util_types.hpp"

static EGLint const attribute_list[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_NONE
};
int main()
{
    
    struct {
        SDL_Window* window;
    } _pvt;
    
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    EGLint num_config;
    
    /* get an EGL display connection */
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display != EGL_NO_DISPLAY);
    assert(eglGetError() == EGL_SUCCESS);
    
    /* initialize the EGL display connection */
    auto r1 = eglInitialize(display, NULL, NULL);
    assert(r1 == EGL_TRUE);
    assert(eglGetError() == EGL_SUCCESS);
    
    
    /* get an appropriate EGL frame buffer configuration */
    auto r2 = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    assert(r2 == EGL_TRUE);
    assert(eglGetError() == EGL_SUCCESS);
    
    /* create an EGL rendering context */
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    assert(context != EGL_NO_CONTEXT);
    assert(eglGetError() == EGL_SUCCESS);
    
    
    auto r9 = SDL_Init(SDL_INIT_VIDEO);
    assert(!r9);
    assert(eglGetError() == EGL_SUCCESS);
    
    auto initial_width = 100;
    auto initial_height = 100;
    char* title = "ASDFASDF";
    
    _pvt.window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        initial_width, initial_height, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    assert(_pvt.window);
    assert(eglGetError() == EGL_SUCCESS);
    
    
    SDL_SysWMinfo window_info;
    
    SDL_VERSION(&window_info.version);
    
    auto r3 = SDL_GetWindowWMInfo(_pvt.window,&window_info);
    assert(r3 == SDL_TRUE);
    
    assert(window_info.subsystem == SDL_SYSWM_X11);
    
    //window_info.x11.display;
    
    /* create an EGL window surface */
    surface = eglCreateWindowSurface(display, config, 
                                     (EGLNativeWindowType)window_info.info.x11.window, NULL);
    assert(surface != EGL_NO_SURFACE);
    assert(eglGetError() == EGL_SUCCESS);
    
    /* connect the context to the surface */
    eglMakeCurrent(display, surface, surface, context);
    
    assert(eglGetError() == EGL_SUCCESS);
    assert(glGetError() == GL_NO_ERROR);
    
    /* clear the color buffer */
    glClearColor(1.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    
    assert(glGetError() == GL_NO_ERROR);
    assert(eglGetError() == EGL_SUCCESS);
    
    eglSwapBuffers(display, surface);
    assert(glGetError() == GL_NO_ERROR);
    assert(eglGetError() == EGL_SUCCESS);
    
    sleep(10);
    return EXIT_SUCCESS;
}

