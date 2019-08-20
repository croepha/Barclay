// Copyright 2019 David Butler <croepha@gmail.com>

#if 0 // Example usage:

#include <stdio.h>

#define NO_DEFINE_IMPLEMENTATION
#define DEFINE_SINGLETONS
#include "util_lazy_sdlgl.cpp"
#include "render_ascii_mono.cpp"

#undef NO_DEFINE_IMPLEMENTATION
#undef DEFINE_SINGLETONS


int main () {
    
    sdlgl_init();
    
    render_ascii_mono_init();
    auto font = render_ascii_mono_load_font("assets/liberation-mono.ttf", 25);
    
    printf("asdfasdf\n");
    
    
    int text_x = 50;
    int text_y = 50;
    
    for (;;) {
        
        sldgl_frame();
        
        StackBuffer<1<<20> _render_buffer;
        render_ascii_mono_init_buffer(_render_buffer);
        
        String _s = {
            .start = "ASDFASsdasfasdDFAD",
        }; _s.len = strlen(_s.start);
        
        render_ascii_mono_line(_render_buffer, debug_font, 100, 100, _s, 1);
        render_ascii_mono_flush(_render_buffer, debug_font);
        
    }
    
}


#endif


#if defined(_GLSL_VERTEX)
attribute vec2 coord_vertex;
attribute vec2 coord_atlas;
//attribute lowp float color_index;

varying vec2 coord_atlas_f;
uniform vec2 dims_screen;
uniform float point_height;

//uniform lowp sampler2D color_palette;

varying vec4 color_f;

void main(){
    
    ivec2 atlas_glyph_size_px = ivec2(13, 25);
    
    coord_atlas_f = coord_atlas / 1024.;
    gl_PointSize = point_height;    
    //color_f = texture2D(color_palette, vec2(color_index/256.0, 0.5));
    color_f = vec4(1.0,1.0,1.0,1.0);
    
    gl_Position.x =  coord_vertex.x * (2.0/dims_screen.x) - 1.0;
    gl_Position.y = -coord_vertex.y * (2.0/dims_screen.y) + 1.0;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    
}

#elif defined(_GLSL_FRAGMENT)
uniform sampler2D atlas;
uniform vec2       atlas_params;
varying vec2 coord_atlas_f;
varying vec4 color_f;

void main(){
    gl_FragColor.rbg = color_f.rgb;
    vec2 gp = (gl_PointCoord - vec2(0.25,0)) * atlas_params;
    gl_FragColor.a = texture2D(atlas, coord_atlas_f + gp).r * color_f.a;
}

#else 
#include "util_types.hpp"

static const u16 font_bake_width   = 1024;
static const u16 font_bake_height  = 1024;
static const u16 font_bake_padding = 10;
static const u16 font_bake_margin  = 10;


struct render_ascii_mono_Font { 
    u32  gl_texture;
    u16  sw, sh;
    
    u16 _arc() { return (font_bake_width+font_bake_margin)/(sw+font_bake_padding); } // Number of sprites in a row
    u16 sprite_x(u16 i) { return font_bake_margin + (i%_arc())*(sw+font_bake_padding);}
    u16 sprite_y(u16 i) { return font_bake_margin + (i/_arc())*(sh+font_bake_padding);}
};

render_ascii_mono_Font render_ascii_mono_load_font(char*font_path, u16 font_height);
void render_ascii_mono_init();
void render_ascii_mono_init_buffer(Buffer buffer);
void render_ascii_mono_character(Buffer buffer, render_ascii_mono_Font font, s16 x, s16 y, char c, u8 color);
void render_ascii_mono_flush(Buffer buffer, render_ascii_mono_Font font);
void render_ascii_mono_line(Buffer buffer, render_ascii_mono_Font&font, s16 x, s16 y, String string , u8 color);


#if !defined(NO_DEFINE_IMPLEMENTATION)

#define NO_DEFINE_IMPLEMENTATION
#include "util_gl.cpp"
#undef NO_DEFINE_IMPLEMENTATION
#include "stb_truetype.h"
#include "util_types.hpp"
#include "util_lazy_io.hpp"
void _gl_check_shader(u32 shader, char* file_path);
void _gl_check_program(u32 program);
void _gl_make_texture_mask(u32&texture_name, s32 w, s32 h, void*pixels);
static u32 vertex_array_buffer;
static u32 program;





struct VertData {
    s16 v[2]; s16 t[2]; u8 color;
};

struct BufferData {
    u32           vert_count;
    VertData verts[0];
};


void render_ascii_mono_flush(Buffer buffer, render_ascii_mono_Font font) {
    auto&_bd = *(BufferData*)buffer.start;
    
    float atlas_params_x = (float)font.sh/(float)font_bake_width;
    float atlas_params_y = (float)font.sh/(float)font_bake_height;
    
#if defined(USING_GL2)
    glEnable(GL_POINT_SPRITE);
#endif
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_array_buffer);
    
    glUniform1i(glGetUniformLocation(program,"atlas"),0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.gl_texture);
    
    auto _shloc_point_h = glGetUniformLocation(program, "point_height");
    assert(_shloc_point_h != -1);
    glUniform1f(_shloc_point_h, font.sh);
    
    auto _shloc_atlas_params = glGetUniformLocation(program, "atlas_params");
    assert(_shloc_atlas_params != -1);
    glUniform2f(_shloc_atlas_params, atlas_params_x, atlas_params_y);
    
    auto _shloc_dims_screen = glGetUniformLocation(program, "dims_screen");
    assert(_shloc_dims_screen != -1);
    glUniform2f(_shloc_dims_screen, 1024, 768);
    
    
    auto _shloc_coord_vertex = glGetAttribLocation(program, "coord_vertex");
    assert(_shloc_coord_vertex != -1);
    glEnableVertexAttribArray((u32)_shloc_coord_vertex);
    glVertexAttribPointer(
        (u32)_shloc_coord_vertex, 2, GL_SHORT , 0, 
        sizeof(VertData), (void*)offsetof(VertData, v));
    
    
    auto _shloc_coord_atlas = glGetAttribLocation(program, "coord_atlas");
    assert(_shloc_coord_atlas != -1);
    glEnableVertexAttribArray((u32)_shloc_coord_atlas);
    glVertexAttribPointer(
        (u32)_shloc_coord_atlas, 2, GL_SHORT , 0, 
        sizeof(VertData), (void*)offsetof(VertData, t));
    
    
    glBufferData(GL_ARRAY_BUFFER, (s64)(sizeof(*_bd.verts)* _bd.vert_count), _bd.verts, GL_STREAM_DRAW);
    glDrawArrays(GL_POINTS, 0, (s32)_bd.vert_count);
    
    _bd.vert_count = 0;
    
}


void render_ascii_mono_character(Buffer buffer, render_ascii_mono_Font font, s16 x, s16 y, char c, u8 color) {
    auto&_bd = *(BufferData*)buffer.start;
    
    if (_bd.vert_count >= buffer.len / sizeof(*_bd.verts)) {
        render_ascii_mono_flush(buffer, font);
    }
    
    auto&v=_bd.verts[_bd.vert_count++];
    
    v.v[0] = x;
    v.v[1] = y;
    v.t[0] = font.sprite_x((u16)c) * 1024 / (s16)font_bake_width;
    v.t[1] = font.sprite_y((u16)c) * 1024 / (s16)font_bake_height;
    v.color = (u8)color;
    
}

void render_ascii_mono_init_buffer(Buffer buffer) { 
    auto&_bd = *(BufferData*)buffer.start;
    assert(buffer.len > sizeof(BufferData));
    _bd.vert_count = 0;
}

void render_ascii_mono_init() {
    
    program = glCreateProgram();
    
    {   auto _sh = glCreateShader(GL_VERTEX_SHADER);
        auto _b = load_file_lazy(__FILE__);
        char* _s[3] = {
            "#version 120\n",
            "#define _GLSL_VERTEX\n",
            (char*)_b.start,
        };
        glShaderSource(_sh, 3, _s, 0);
        glCompileShader(_sh);
        free(_b.start);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    {   auto _sh = glCreateShader(GL_FRAGMENT_SHADER);
        auto _b = load_file_lazy(__FILE__);
        char* _s[3] = {
            "#version 120\n",
            "#define _GLSL_FRAGMENT\n",
            (char*)_b.start,
        };
        glShaderSource(_sh, 3, _s, 0);
        glCompileShader(_sh);
        free(_b.start);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    
    glLinkProgram(program);
    glValidateProgram(program);
    _gl_check_program(program);
    
    glUseProgram(program);
    
    glGenBuffers(1, &vertex_array_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_array_buffer);
    
}

render_ascii_mono_Font render_ascii_mono_load_font(char*font_path, u16 font_height) {
    
    
    auto ttf_buffer = load_file_lazy(font_path);
    auto temp_bitmap = (u8*)malloc(font_bake_width*font_bake_height);
    assert(temp_bitmap);
    
    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, (u8*)ttf_buffer.start, stbtt_GetFontOffsetForIndex((u8*)ttf_buffer.start,0));
    
    auto scale = stbtt_ScaleForPixelHeight(&font_info, font_height);
    int advance_width, left_side_bearing;
    stbtt_GetCodepointHMetrics(&font_info, 1, &advance_width, &left_side_bearing);
    
    int ascent;
    stbtt_GetFontVMetrics(&font_info, &ascent, 0, 0);
    
    
    auto font_width  = (u16)(scale * advance_width);
    
    render_ascii_mono_Font font = {};
    
    font.sw = font_width;
    font.sh = font_height;
    
    
    
    assert(scale * advance_width * 16 < font_bake_width );
    assert(        font_height    * 16 < font_bake_height);
    
    for (u16 char_i=0; char_i<256; char_i++) {
        
        int atlas_x = font.sprite_x(char_i);
        int atlas_y = font.sprite_y(char_i);
        
        int x0,y0,x1,y1;
        stbtt_GetCodepointBitmapBox(&font_info, char_i, scale,scale, &x0,&y0,&x1,&y1);
        
        
        atlas_x = atlas_x + x0;
        atlas_y = atlas_y + y0 + (int)(ascent*scale);
        
        u8* base_pixel = temp_bitmap +  atlas_y * font_bake_width + atlas_x;
        
        stbtt_MakeCodepointBitmap(&font_info, base_pixel, x1-x0, y1-y0,
                                  font_bake_width, scale, scale, char_i);
    }
    
    _gl_make_texture_mask(font.gl_texture, font_bake_width, font_bake_height, temp_bitmap);
    free(ttf_buffer.start);
    free(temp_bitmap);
    
    return font;
    
    
}

void render_ascii_mono_line(Buffer buffer, render_ascii_mono_Font&font, s16 x, s16 y, String string , u8 color) {
    
    for(u32 i=0; i<string.len; i++) {
        render_ascii_mono_character(buffer, font, x, y, string.start[i], color);
        x += font.sw;
    }
}



#endif
#endif






