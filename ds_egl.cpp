// Copyright 2019 David Butler <croepha@gmail.com>

#include <stdio.h>

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GL/gl.h>
#include <GL/glext.h>


#include "util_types.hpp"



EGLDisplay egl_display;
EGLSurface egl_surface;

static void egl_debug_proc(
EGLenum error,
const char *command,
EGLint messageType,
EGLLabelKHR threadLabel,
EGLLabelKHR objectLabel,
const char* message) {
    
    printf("EGL_DEBUG_PROC: %d %s %d %p %p %s\n",
           error, command, messageType, threadLabel, objectLabel, message
           );
    
    assert(0);
}


void egl_init1() {
    auto eglDebugMessageControl = (typeof(&eglDebugMessageControlKHR))
        eglGetProcAddress("eglDebugMessageControlKHR");
    assert(eglDebugMessageControl);
    auto r7 = eglDebugMessageControl(egl_debug_proc, 0);
    assert(r7 == EGL_SUCCESS);
}

void egl_init2() {
    
    int egl_major = -1;
    int egl_minor = -1;
    
    eglInitialize(egl_display, &egl_major, &egl_minor);
    assert(egl_major* 1000  + egl_minor >=  1004);
    
    
    printf("EGL_CLIENT_APIS:%s EGL_VENDOR:%s EGL_VERSION:%s EGL_EXTENSIONS:%s \n",
           eglQueryString(egl_display, EGL_CLIENT_APIS),
           eglQueryString(egl_display, EGL_VENDOR),
           eglQueryString(egl_display, EGL_VERSION),
           eglQueryString(egl_display, EGL_EXTENSIONS)
           );
    
    //eglBindAPI(EGL_OPENGL_API); // OPENGGL
    eglBindAPI(EGL_OPENGL_ES_API); // OPENGLES
    
    assert(eglGetError() == EGL_SUCCESS);
    
}



void egl_init3(void* native_window) {
    EGLConfig config;
    EGLint num_config;
    
    EGLint config_attributes[] = {
#if 1
        EGL_CONFORMANT     , EGL_OPENGL_BIT, // OPENGL 2.1
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, // OPENGL 2.1
#endif
#if 0
        EGL_CONFORMANT     , EGL_OPENGL_ES_BIT, // OPENGLES 2.0
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, // OPENGLES 2.0
#endif
        
        
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_NONE
    };
    
    
    eglChooseConfig(egl_display, config_attributes, &config, 1, &num_config);
    
    EGLint context_attributes[] = { 
#if 0
        EGL_CONTEXT_MAJOR_VERSION_KHR, 2, // OPENGL 2.1
        EGL_CONTEXT_MINOR_VERSION_KHR, 1, 
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
#endif
#if 0
        EGL_CONTEXT_MAJOR_VERSION_KHR, 3, // OPENGL 3.3
        EGL_CONTEXT_MINOR_VERSION_KHR, 3, 
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
#endif
#if 1
        EGL_CONTEXT_MAJOR_VERSION_KHR, 2, // OPENGLES 2.0
        EGL_CONTEXT_MINOR_VERSION_KHR, 0, // OPENGLES 2.0
#endif
        
        EGL_CONTEXT_FLAGS_KHR,  EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
        EGL_NONE,
    };
    
    auto egl_context = eglCreateContext(
        egl_display, config, 
        EGL_NO_CONTEXT, context_attributes);
    auto r5 = eglGetError();
    assert(r5 == EGL_SUCCESS);
    assert(eglGetError() == EGL_SUCCESS);
    assert(egl_context != EGL_NO_CONTEXT);
    
    egl_surface = eglCreateWindowSurface(egl_display, config, 
                                         (EGLNativeWindowType)native_window, NULL);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(egl_display, egl_surface);
    
    
}

void egl_frame() {
    glFlush();
    eglSwapBuffers(egl_display, egl_surface);
    assert(eglGetError() == EGL_SUCCESS);
    
}

