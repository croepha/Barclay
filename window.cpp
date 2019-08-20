// Copyright 2019 David Butler <croepha@gmail.com>

#define _ANON_START namespace {
#define _ANON_END   }


#include <string.h>

#include <easy/profiler.h>

#include "common.hpp"
#include "util_misc.hpp"

#define NO_DEFINE_IMPLEMENTATION

#include "util_gl.cpp"
//#include "util_lazy_sdlgl.cpp"
//#include "render_ascii_mono.cpp"
#include "render_draw_texture.cpp"
#include "render_boxes.cpp"

#undef NO_DEFINE_IMPLEMENTATION


// #include <linux/input-event-codes.h>
#define BTN_LEFT                0x110



extern V2<s16> screen_mouse_pos;
extern V2<s16> screen_mouse_pos_last;
extern u16 frame_time_delta_ms;

bool keymap_is_pressed(u16 scancode);
bool keymap_is_pressed_this_frame(u16 scancode);
void server_frame_io();
Buffer keymap_frame();
bool _debug_should_quit;
void server_frame_update_clients();
void ds_common_frame();

#include <stdio.h>



_ANON_START



//render_ascii_mono_Font debug_font;


void mouse_in_box(bool*mouse_inside, Bounds bounds) {
    
    auto _mx = screen_mouse_pos_last.x;
    auto _my = screen_mouse_pos_last.y;
    
    if (mouse_inside &&
        _mx >= bounds.x && 
        _mx < bounds.x+bounds.w  &&
        _my >= bounds.y   &&
        _my < bounds.y+bounds.h) {
        *mouse_inside = 1;
    }
}


void draw_box(Bounds bounds, u32 color) {
    
    ::draw_box((s16)bounds.x,
               (s16)bounds.y,
               (s16)bounds.w,
               (s16)bounds.h,color);
}




// __attribute__((no_sanitize("address")))
void blit_client(Client&client) {
    
    s16 border_width = 4;
    s16 title_bar_height = 12;
    
    s16 content_x = client.pos.x + border_width;
    s16 content_y = client.pos.y + border_width + title_bar_height;
    
    //draw_box(0, content_x, content_y, client.w, client.h, 0x55555555);
    render_draw_texture(content_x, content_y, client.size.w, client.size.h, client.gl_texture);
    
    
#if 0
    for (s32 px = 0; px<client.w; px++) {
        for (s32 py = 0; py<client.h; py++) {
            
            auto sx = px + content_x;
            auto sy = py + content_y;
            
            if (sx <     0 ) continue;
            if (sy <     0 ) continue;
            if (sx >= 1024 ) continue;
            if (sy >=  768 ) continue;
            
            
            auto server_row   = (u32*)(void*)(((u8*)server_surface->pixels) + server_surface->pitch * sy);
            auto server_pixel = server_row + sx;
            
            ClientBitmapSpec _spec = {.w = client.w, .h=client.h};
            
            auto client_row   = (u32*)(void*)(((u8*)client.bitmap_front) + _spec.pitch() * py);
            auto client_pixel = client_row + px;
            
            
            auto pix_value = *client_pixel;
            
            *server_pixel = pix_value;
            
        }
    }
#endif
    
}




_ANON_END



void window_init() {
    
    screen_mouse_pos = {1024/2, 768/2};
    draw_box_init();
    render_draw_texture_init();
    //render_ascii_mono_init();
    //debug_font = render_ascii_mono_load_font("assets/liberation-mono.ttf", 25);
}




s16 border_width = 4;
s16 title_bar_height = 12;
s16 handle_width =  32;


bool hide_cursor;


extern char scancode_input_buffer[];
extern  int scancode_input_buffer_count;


Buffer text_input_buffer;
void* click_hot_item;


void window_frame_update() {
    
    if (!keymap_is_pressed(BTN_LEFT)) {
        click_hot_item = 0;
    }
    
    EASY_FUNCTION();
    
    hide_cursor = 0;
    
    for (auto i = clients.last; i; i=clients.prev(i)) {
        auto&client = *i;
        
        s16 outer_w   = client.size.w + border_width * 2; 
        //s32 outer_h   = client.h + border_width * 2  + title_bar_height;
        
        Bounds bounds_border_top = { client.pos.x, client.pos.y        , outer_w, border_width};
        Bounds bounds_border_bottom = { client.pos.x, client.pos.y+border_width+title_bar_height+client.size.h, outer_w,  border_width };
        Bounds bounds_border_left = { client.pos.x, client.pos.y+border_width, border_width, client.size.h + title_bar_height};
        Bounds bounds_border_right = { client.pos.x+border_width+client.size.w, client.pos.y+border_width, border_width, client.size.h + title_bar_height };
        Bounds bounds_handle_tlt = {client.pos.x, client.pos.y, handle_width, border_width};
        Bounds bounds_handle_tll = {client.pos.x, client.pos.y, border_width, handle_width};
        Bounds bounds_handle_trt = { client.pos.x+client.size.w+2*border_width-handle_width, client.pos.y, handle_width, border_width };
        Bounds bounds_handle_trr = { client.pos.x+client.size.w+2*border_width-border_width, client.pos.y, border_width, handle_width };
        Bounds bounds_handle_blb = { client.pos.x, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-border_width, 
            handle_width, border_width };
        Bounds bounds_handle_bll = { client.pos.x, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-handle_width, 
            border_width, handle_width };
        Bounds bounds_handle_brb = { client.pos.x+client.size.w+2*border_width-handle_width, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-border_width, 
            handle_width, border_width}; 
        Bounds bounds_handle_brr = { client.pos.x+client.size.w+2*border_width-border_width, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-handle_width, 
            border_width, handle_width };
        Bounds bounds_title = { client.pos.x+border_width, client.pos.y+border_width, client.size.w, title_bar_height };
        Bounds bounds_client = { client.pos.x+border_width, client.pos.y+border_width+title_bar_height,
            client.size.w, client.size.h };
        
        
        
        
        bool mouse_over_title  = 0;
        bool mouse_over_left   = 0;
        bool mouse_over_right  = 0;
        bool mouse_over_top    = 0;
        bool mouse_over_bottom = 0;
        bool mouse_over_tl     = 0;
        bool mouse_over_tr     = 0;
        bool mouse_over_bl     = 0;
        bool mouse_over_br     = 0;
        bool mouse_over_client = 0;
        
        mouse_in_box(&mouse_over_top   , bounds_border_top);
        mouse_in_box(&mouse_over_bottom, bounds_border_bottom);
        mouse_in_box(&mouse_over_left  , bounds_border_left);
        mouse_in_box(&mouse_over_right , bounds_border_right);
        mouse_in_box(&mouse_over_tl    , bounds_handle_tlt);
        mouse_in_box(&mouse_over_tl    , bounds_handle_tll);
        mouse_in_box(&mouse_over_tr    , bounds_handle_trt);
        mouse_in_box(&mouse_over_tr    , bounds_handle_trr);
        mouse_in_box(&mouse_over_bl    , bounds_handle_blb);
        mouse_in_box(&mouse_over_bl    , bounds_handle_bll);
        mouse_in_box(&mouse_over_br    , bounds_handle_brb);
        mouse_in_box(&mouse_over_br    , bounds_handle_brr);
        mouse_in_box(&mouse_over_title , bounds_title);
        mouse_in_box(&mouse_over_client, bounds_client);
        
        
        
        
        if (mouse_over_tr) {
            mouse_over_top    = 1;
            mouse_over_right  = 1;
        }
        if (mouse_over_tl) {
            mouse_over_top    = 1;
            mouse_over_left   = 1;
        }
        
        if (mouse_over_br) {
            mouse_over_bottom = 1;
            mouse_over_right  = 1;
        }
        
        if (mouse_over_bl) {
            mouse_over_bottom = 1;
            mouse_over_left   = 1;
        }
        
        bool hit_window = 0;
        
        
        if (keymap_is_pressed_this_frame(BTN_LEFT) || click_hot_item == &client ) {
            
            auto mouse_y = screen_mouse_pos.y;
            auto mouse_x = screen_mouse_pos.x;
            auto mouse_y_start = screen_mouse_pos_last.y;
            auto mouse_x_start = screen_mouse_pos_last.x;
            
            
            if (mouse_over_top) {
                
                client.pos.y += mouse_y - mouse_y_start;
                client.size.h -= mouse_y - mouse_y_start;
                client.resized = 1;
                hit_window = 1;
                
            } 
            if (mouse_over_bottom) {
                
                client.size.h += mouse_y - mouse_y_start;
                client.resized = 1;
                hit_window = 1;
            } 
            if (mouse_over_left) {
                
                client.pos.x += mouse_x - mouse_x_start;
                client.size.w -= mouse_x - mouse_x_start;
                client.resized = 1;
                hit_window = 1;
                
            } 
            if (mouse_over_right) {
                
                client.size.w += mouse_x - mouse_x_start;
                client.resized = 1;
                hit_window = 1;
                
            } 
            if (mouse_over_title) {
                client.pos.x += mouse_x - mouse_x_start;
                client.pos.y += mouse_y - mouse_y_start;
                hit_window = 1;
            }
            
            if (mouse_over_client) {
                hit_window = 1;
            }
            
            
        }
        
        if (client.size.w < 50) {
            client.size.w = 50;
            client.resized = 1;
        }
        if (client.size.h < 50) {
            client.size.h = 50;
            client.resized = 1;
        }
        
        
        if (&client == clients.last && mouse_over_client) {
            hide_cursor = 1;
        }
        
        
        if (hit_window) { 
            click_hot_item = &client;
            if (&client != clients.last) {
                clients.remove(&client);
                clients.append(&client);
            }
            break;
        }
        
        
        
    }
    
    
    for (auto&client:clients) {
        if (&client == clients.last) {
            client.has_focus = 1;
            client.new_text_input = text_input_buffer;
            client.new_scancode_input = { scancode_input_buffer, (size_t)scancode_input_buffer_count*2};
        } else {
            client.has_focus = 0;
            client.new_text_input = {};
            client.new_scancode_input = {};
        }
        
        V2<s16> _offset = { border_width, (s16)(border_width + title_bar_height) };
        client.mouse_pos = screen_mouse_pos - client.pos - _offset;
    }
}




void window_frame_render() {
    
    EASY_FUNCTION();
    
    
    glClear(GL_COLOR_BUFFER_BIT);
    
#if 0
    {
        
        StackBuffer<1<<20> _render_buffer;
        render_ascii_mono_init_buffer(_render_buffer);
        
        String _s = {
            .start = "ASDFASsdasfasdDFAD",
        }; _s.len = strlen(_s.start);
        
        render_ascii_mono_line(_render_buffer, debug_font, 100, 100, _s, 1);
        
        render_ascii_mono_flush(_render_buffer, debug_font);
        
    }
    
#endif
    
    
    for (auto&client:clients) {
        
        s32 outer_w   = client.size.w + border_width * 2; 
        //s32 outer_h   = client.h + border_width * 2  + title_bar_height;
        
        
        Bounds bounds_border_top = { client.pos.x, client.pos.y        , outer_w, border_width};
        Bounds bounds_border_bottom = { client.pos.x, client.pos.y+border_width+title_bar_height+client.size.h, outer_w,  border_width };
        Bounds bounds_border_left = { client.pos.x, client.pos.y+border_width, border_width, client.size.h + title_bar_height};
        Bounds bounds_border_right = { client.pos.x+border_width+client.size.w, client.pos.y+border_width, border_width, client.size.h + title_bar_height };
        Bounds bounds_handle_tlt = {client.pos.x, client.pos.y, handle_width, border_width};
        Bounds bounds_handle_tll = {client.pos.x, client.pos.y, border_width, handle_width};
        Bounds bounds_handle_trt = { client.pos.x+client.size.w+2*border_width-handle_width, client.pos.y, handle_width, border_width };
        Bounds bounds_handle_trr = { client.pos.x+client.size.w+2*border_width-border_width, client.pos.y, border_width, handle_width };
        Bounds bounds_handle_blb = { client.pos.x, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-border_width, 
            handle_width, border_width };
        Bounds bounds_handle_bll = { client.pos.x, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-handle_width, 
            border_width, handle_width };
        Bounds bounds_handle_brb = { client.pos.x+client.size.w+2*border_width-handle_width, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-border_width, 
            handle_width, border_width}; 
        Bounds bounds_handle_brr = { client.pos.x+client.size.w+2*border_width-border_width, 
            client.pos.y+client.size.h+2*border_width+title_bar_height-handle_width, 
            border_width, handle_width };
        Bounds bounds_title = { client.pos.x+border_width, client.pos.y+border_width, client.size.w, title_bar_height };
        
        
        u32 color_grey  = 0xff7f7f7f;
        u32 color_white = 0xffffffff;
        u32 color_green = 0xff00ff00;
        
        u32 color_border = color_grey;
        u32 color_handle = color_green;
        
        
        draw_box(bounds_border_top, color_border);
        draw_box(bounds_border_bottom, color_border);
        draw_box(bounds_border_left, color_border);
        draw_box(bounds_border_right, color_border);
        draw_box(bounds_handle_tlt, color_handle);
        draw_box(bounds_handle_tll, color_handle);
        draw_box(bounds_handle_trt, color_handle);
        draw_box(bounds_handle_trr, color_handle);
        draw_box(bounds_handle_blb, color_handle);
        draw_box(bounds_handle_bll, color_handle);
        draw_box(bounds_handle_brb, color_handle);
        draw_box(bounds_handle_brr, color_handle);
        draw_box(bounds_title, color_white);
        
        blit_client(client);
        
    }
    
    
    if (!hide_cursor) {
        
        Bounds bounds_cursor = {(s16)screen_mouse_pos.x, (s16)screen_mouse_pos.y, 5, 5};
        draw_box(bounds_cursor, 0xffff0000);
    }
    
}


void window_loop() {
    
    
    while (!_debug_should_quit) {
        EASY_BLOCK("window_loop");
        
        
        //printf("START %u\n", get_ticks());
        server_frame_io();
        ds_common_frame();
        window_frame_update();
        
        text_input_buffer = keymap_frame();
        
        server_frame_update_clients();
        
        
        //printf("AFTER IO %u\n", get_ticks());
        window_frame_render();
        //printf("AFTER RENDER %u\n", get_ticks());
        
        //printf("AFTER FLIP %u\n", get_ticks());
        
        
        
        
    }
}