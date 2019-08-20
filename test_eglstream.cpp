// Copyright 2019 David Butler <croepha@gmail.com>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NO_DEFINE_IMPLEMENTATION

#include "util_gl.cpp"
#include "render_draw_texture.cpp"

#undef NO_DEFINE_IMPLEMENTATION


void ds_platform_init();
void ds_platform_frame();
bool _debug_ignoring_events;
bool _debug_should_quit;

int main () {
    
    ds_platform_init();
    
    
    printf("GL_STRINGS: Vendor:%s Renderer:%s Version:%s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)); 
    _gl_init();
    render_draw_texture_init();
    
    u32 texture_name;
    s16 size = 400;
    
    {
        auto pixels = (u32*)malloc((u32)size*(u32)size*sizeof(u32));
        memset(pixels, 0, (u32)size*(u32)size*sizeof(u32));
        
        auto _px = [&](int x, int y) -> u32& {
            static u32 _dummmy;
            if (x<0 || x>= size ||
                y<0 || y>= size ) return _dummmy;
            
            return *(pixels + y * size + x);
        };
        
        
        for (int i=0;i<size/2;i++) {
            for (int iw=0;iw<10;iw++) {
                _px(i+1, size/2 - i + iw) = 0xFFFFFFFF;
            }
        }
        for (int i=0;i<size/2;i++) {
            for (int iw=0;iw<10;iw++) {
                _px(size - i-1, size/2 - i + iw) = 0xFFFFFFFF;
            }
        }
        glGenTextures(1, &texture_name);
        glBindTexture(GL_TEXTURE_2D, texture_name);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        free(pixels);
        
        
    }
    
    while (!_debug_should_quit) {
        ds_platform_frame();
        glClear(GL_COLOR_BUFFER_BIT);
        render_draw_texture(20, 20, size, size, texture_name);
    }
    
    
}
