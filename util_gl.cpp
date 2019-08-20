// Copyright 2019 David Butler <croepha@gmail.com>

#include "util_types.hpp"

//#define USING_GL2
#define USING_GL3
//#define USING_GLES2


#define GL_GLEXT_PROTOTYPES


#if defined(USING_GL2) || defined(USING_GL3)
#include <GL/gl.h>
#include <GL/glext.h>
//#define GL_DEBUG_SUFFIX
#define glDebugMessageCallback      glDebugMessageCallbackARB
#define _GL_MASK_TYPE GL_RED


#endif


#ifdef USING_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_DEBUG_SUFFIX _KHR
#define glDebugMessageCallback      glDebugMessageCallbackKHR
#define GL_DEBUG_OUTPUT             GL_DEBUG_OUTPUT_KHR
#define GL_DEBUG_OUTPUT_SYNCHRONOUS GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR
#define _GL_MASK_TYPE GL_ALPHA

#endif

extern bool _gl_had_error;
void _gl_init();
void _gl_check_shader(u32 shader, char* file_path);
void _gl_check_program(u32 program);
void _gl_make_texture_mask(u32&texture_name, s32 w, s32 h, void*pixels);


#if !defined(NO_DEFINE_IMPLEMENTATION)
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "util_types.hpp"

bool _gl_had_error = 0;
bool _gl_shader_had_error = 0;

void _gl_debug_callback(GLenum source, GLenum type, GLuint id,
                        GLenum severity, GLsizei /*length*/, 
                        const GLchar* message, const void* /*userParam*/) {
    if (type == GL_DEBUG_TYPE_OTHER &&
        severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    
    char* source_string = 0;
    switch (source) { default: assert(0); break;
        case GL_DEBUG_SOURCE_API             : source_string = "API"    ; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM   : source_string = "OS"     ; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER : source_string = "SHADER" ; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY     : source_string = "CONTRIB"; break;
        case GL_DEBUG_SOURCE_APPLICATION     : source_string = "APP"    ; break;
        case GL_DEBUG_SOURCE_OTHER           : source_string = "OTHER"  ; break;
    }
    char* severity_string = 0;
    switch (severity) { default: assert(0);  break;
        case GL_DEBUG_SEVERITY_HIGH        : severity_string = "HIGH"  ; break;
        case GL_DEBUG_SEVERITY_MEDIUM      : severity_string = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_LOW         : severity_string = "LOW"   ; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_string = "NOTE"  ; break;
    }
    
    char* type_string = 0;    
    switch (type) { default: assert(0);  break;
        case GL_DEBUG_TYPE_ERROR              : type_string = "ERROR"       ; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_string = "DEPRECATED"  ; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : type_string = "UNDEFINED"   ; break;
        case GL_DEBUG_TYPE_PORTABILITY        : type_string = "PORTABILITY" ; break;
        case GL_DEBUG_TYPE_PERFORMANCE        : type_string = "PERFORMANCE" ; break;
        case GL_DEBUG_TYPE_MARKER             : type_string = "MARKER"      ; break;
        case GL_DEBUG_TYPE_PUSH_GROUP         : type_string = "PUSH"        ; break;
        case GL_DEBUG_TYPE_POP_GROUP          : type_string = "POP"         ; break;
        case GL_DEBUG_TYPE_OTHER              : type_string = "OTHER"       ; break;
    }
    
    bool suppress = 0;
    
    
    
    if (type == GL_DEBUG_TYPE_PERFORMANCE && 
        strstr(message, "generating temporary index buffer for drawing PIPE_PRIM_TRIANGLE_FAN")) {
        suppress= 1;
    }
    
    if (source == GL_DEBUG_SOURCE_SHADER_COMPILER){
        
        _gl_shader_had_error = 1;
        suppress = 1;
    }
    
    if (strstr(message, "being recompiled based on GL state.")) {
        suppress = 1;
    }
    
    
    if (!suppress) {
        
        fprintf(stderr, "GL_DEBUG: %s:%s:%s: `%s' %u\n",
                source_string, type_string, severity_string, message, id);
        fflush(stderr);
        assert(0);
    }
    
    
}


void _gl_print_info_log(char* log, char* file_path) {
#if 0
    bool at_line_start = true;
    for (char*c = log; *c; c++) {
        if (at_line_start) { 
            printf("%s:", file_path);
            assert(*c++ == '0');
            assert(*c++ == '(');
            while(isdigit(*c)) {
                printf("%c", *c++);
            }
            assert(*c++ == ')');
            
            c++;
        }
        printf("%c", *c);
        at_line_start = *c == '\n';
    }
#else
    printf("_gl_print_info_log:%s\n%s",file_path, log);
#endif
    printf("\n");
    fflush(stdout);
}
void _gl_check_shader(u32 shader, char* file_path) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        assert(length);
        auto log = (char*)malloc((u32)length + 1);
        glGetShaderInfoLog(shader, length, &length, log);
        log[length-1] = 0;
        _gl_had_error = true;
        _gl_print_info_log(log, file_path);
    }
}

void _gl_check_program(u32 program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        assert(length);
        auto log = (char*)malloc((u32)length+1);
        glGetProgramInfoLog(program, length, &length, log);
        log[length-1]=0;
        printf("PROGRAM LOG START\n");
        printf("%s",log);
        printf("PROGRAM LOG END\n");
        fflush(stdout);
        assert(0);
    }
}


void _gl_make_texture_mask(u32&texture_name, s32 w, s32 h, void*pixels) {
    glGenTextures(1, &texture_name);
    glBindTexture(GL_TEXTURE_2D, texture_name);
    glTexImage2D(GL_TEXTURE_2D, 0, _GL_MASK_TYPE, w, h, 0,
                 _GL_MASK_TYPE, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    
}

void _gl_init() {
    
    
#if 1
    glDebugMessageCallback(_gl_debug_callback, 0);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
    
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    
    
}



#endif





