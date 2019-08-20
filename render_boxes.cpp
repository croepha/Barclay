// Copyright 2019 David Butler <croepha@gmail.com>

#if 0 // SHADER_VERTEX_BEGIN
#version 100
uniform   highp float screen_w;
uniform   highp float screen_h;
attribute highp float in_x;
attribute highp float in_y;
attribute highp vec4  in_color;
varying   highp vec4 vert_color;
void main() {
    highp vec2 p1 = vec2(in_x/screen_w, (screen_h-in_y)/screen_h);
    highp vec2 p2 = p1 - 0.5;
    highp vec2 p3 = p2 * 2.0;
    gl_Position = vec4(p3.x, p3.y, 0.0, 1.0);
    vert_color = in_color / 256.0;
}

// SHADER_FRAGMENT_BEGIN
#version 100
varying highp vec4 vert_color;
void main() {
    gl_FragColor = vert_color;
}
// SHADER_END 
#endif 

#include "common.hpp"

void draw_box_init();
void draw_box(s16 x, s16 y, s16 w, s16 h, u32 color);

#if !defined(NO_DEFINE_IMPLEMENTATION)

#define vertex_shader     shader_boxes_vertex
#define fragment_shader   shader_boxes_fragment

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


struct draw_box_AttributeData{
    s16 x, y;
    u32 color;
};


extern bool _gl_shader_had_error;

void draw_box_init() {
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
    
    assert(!_gl_had_error);
    
    
}


void draw_box(s16 x, s16 y, s16 w, s16 h, u32 color) {
    
    
    glUseProgram(program);
    
    glBindBuffer(GL_ARRAY_BUFFER, vertex_array_buffer);
    
    /*
        glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_name);
    */
    
    
    assert(glGetError() == GL_NO_ERROR);
    
    auto _shloc_screen_w = glGetUniformLocation(program, "screen_w");
    auto _shloc_screen_h = glGetUniformLocation(program, "screen_h");
    
    auto _shloc_in_x = glGetAttribLocation(program, "in_x");
    auto _shloc_in_y = glGetAttribLocation(program, "in_y");
    auto _shloc_in_color = glGetAttribLocation(program, "in_color");
    
    assert(_shloc_in_x != -1);
    assert(_shloc_in_y != -1);
    assert(_shloc_in_color != -1);
    
    assert(glGetError() == GL_NO_ERROR);
    
    glUniform1f(_shloc_screen_w, SCREEN_WIDTH);
    glUniform1f(_shloc_screen_h, SCREEN_HEIGHT);
    
    assert(glGetError() == GL_NO_ERROR);
    
    glEnableVertexAttribArray((u32)_shloc_in_x);
    glEnableVertexAttribArray((u32)_shloc_in_y);
    glEnableVertexAttribArray((u32)_shloc_in_color);
    
    glVertexAttribPointer((u32)_shloc_in_x    , 1, GL_SHORT , 0, sizeof(draw_box_AttributeData), (void*)offsetof(draw_box_AttributeData, x) );
    glVertexAttribPointer((u32)_shloc_in_y    , 1, GL_SHORT , 0, sizeof(draw_box_AttributeData), (void*)offsetof(draw_box_AttributeData, y) );
    glVertexAttribPointer((u32)_shloc_in_color, 4, GL_UNSIGNED_BYTE, 0, sizeof(draw_box_AttributeData), (void*)offsetof(draw_box_AttributeData, color) );
    
    s16 xw = x+w;
    s16 yh = y+h;
    
    static const int vert_count = 6;
    draw_box_AttributeData verts[vert_count] = {
        {.x= x  , .y=y  , .color=color},
        {.x= xw , .y=y  , .color=color},
        {.x= x  , .y=yh , .color=color},
        {.x= xw , .y=y  , .color=color},
        {.x= x  , .y=yh , .color=color},
        {.x= xw , .y=yh , .color=color},
    };
    
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(*verts) * vert_count, verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vert_count);
    
}


#endif







