// Copyright 2019 David Butler <croepha@gmail.com>



#if 0 // SHADER_VERTEX_BEGIN
#version 100
uniform   highp float      screen_w;
uniform   highp float      screen_h;
attribute highp float      screen_x;
attribute highp float      screen_y;
attribute highp float      texture_u;
attribute highp float      texture_v;
varying   highp float      vert_texture_u;
varying   highp float      vert_texture_v;
varying   highp vec4       vert_color;
void main() {
    highp vec2 p1 = vec2(screen_x/screen_w, (screen_h-screen_y)/screen_h);
    highp vec2 p2 = p1 - 0.5;
    highp vec2 p3 = p2 * 2.0;
    gl_Position = vec4(p3.x, p3.y, 0.0, 1.0);
    vert_texture_u = texture_u;
    vert_texture_v = texture_v;
}

// SHADER_FRAGMENT_BEGIN
#version 100
varying highp float      vert_texture_u;
varying highp float      vert_texture_v;
uniform  sampler2D  texture0;
void main() {
    gl_FragColor = texture2D(texture0, vec2(vert_texture_u, vert_texture_v));
}
// SHADER_END 
#endif

#include "common.hpp"

void render_draw_texture_init();
void render_draw_texture(s16 x, s16 y, s16 w, s16 h, u32 texture);

#if !defined(NO_DEFINE_IMPLEMENTATION)

#define vertex_shader     shader_draw_texture_vertex
#define fragment_shader   shader_draw_texture_fragment

extern char* vertex_shader;
extern char* fragment_shader;


#define NO_DEFINE_IMPLEMENTATION
#include "util_gl.cpp"
#undef NO_DEFINE_IMPLEMENTATION
#include "util_types.hpp"
#include "util_lazy_io.hpp"
void _gl_check_shader(u32 shader, char* file_path);
void _gl_check_program(u32 program);
static u32 vertex_array_buffer;
static u32 program;

void render_draw_texture(s16 x, s16 y, s16 w, s16 h, u32 texture) {
    
    static const int vert_count = 6;
    
    s16 xw = x+w;
    s16 yh = y+h;
    
    
    struct AttributeData {
        s16 x, y, u, v;
    } verts[vert_count] = {
        {.x= x  , .y=y  ,.u=0,.v=0},
        {.x= xw , .y=y  ,.u=1,.v=0},
        {.x= x  , .y=yh ,.u=0,.v=1},
        {.x= xw , .y=y  ,.u=1,.v=0},
        {.x= x  , .y=yh ,.u=0,.v=1},
        {.x= xw , .y=yh ,.u=1,.v=1},
    };
    
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_array_buffer);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    auto _shloc_screen_w = glGetUniformLocation(program, "screen_w");
    assert(_shloc_screen_w != -1);
    glUniform1f(_shloc_screen_w, SCREEN_WIDTH);
    
    auto _shloc_screen_h = glGetUniformLocation(program, "screen_h");
    assert(_shloc_screen_h != -1);
    glUniform1f(_shloc_screen_h, SCREEN_HEIGHT);
    
    auto _shloc_x = glGetAttribLocation(program, "screen_x");
    assert(_shloc_x != -1);
    glEnableVertexAttribArray((u32)_shloc_x);
    glVertexAttribPointer(
        (u32)_shloc_x    , 1, GL_SHORT , 0, 
        sizeof(AttributeData), (void*)offsetof(AttributeData, x));
    
    auto _shloc_y = glGetAttribLocation(program, "screen_y");
    assert(_shloc_y != -1);
    glEnableVertexAttribArray((u32)_shloc_y);
    glVertexAttribPointer(
        (u32)_shloc_y    , 1, GL_SHORT , 0, 
        sizeof(AttributeData), (void*)offsetof(AttributeData, y));
    
    auto _shloc_u = glGetAttribLocation(program, "texture_u");
    assert(_shloc_u != -1);
    glEnableVertexAttribArray((u32)_shloc_u);
    glVertexAttribPointer(
        (u32)_shloc_u    , 1, GL_SHORT , 0, 
        sizeof(AttributeData), (void*)offsetof(AttributeData, u));
    
    auto _shloc_v = glGetAttribLocation(program, "texture_v");
    assert(_shloc_v != -1);
    glEnableVertexAttribArray((u32)_shloc_v);
    glVertexAttribPointer(
        (u32)_shloc_v    , 1, GL_SHORT , 0, 
        sizeof(AttributeData), (void*)offsetof(AttributeData, v));
    
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(*verts) * vert_count, verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vert_count);
    
    assert(glGetError() == GL_NO_ERROR);
}



extern bool _gl_shader_had_error;

void render_draw_texture_init() {
    
    program = glCreateProgram();
    
    {   auto _sh = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(_sh, 1, &vertex_shader, 0);
        glCompileShader(_sh);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    {   auto _sh = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(_sh, 1, &fragment_shader, 0);
        glCompileShader(_sh);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    
    glLinkProgram(program);
    glValidateProgram(program);
    _gl_check_program(program);
    assert(!_gl_shader_had_error);
    
    glUseProgram(program);
    
    glGenBuffers(1, &vertex_array_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_array_buffer);
    
    
}

#endif




