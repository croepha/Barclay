
/*
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include "barclay.h"


struct BarclayWindow* windows[128];
unsigned int   window_count;


int main () {
    
    
    windows[window_count++] = barclay_window_init(0);
    
    unsigned char blue = 0;
    for (;;) {
        blue++;
        unsigned int window_i;
        for (window_i=0;window_i<window_count;window_i++) {
            
            struct BarclayWindow* window = windows[window_i];
            
            char close_this_window = 0;
            char spawn_new_window = 0;
            
            if (window->input_text_len || window->input_scan_codes_len) {
                printf("got text: %d `%.*s'  scanncodes: %d: ",
                       window->input_text_len,
                       window->input_text_len, window->input_text,
                       window->input_scan_codes_len);
                unsigned int i;
                for (i=0;i<window->input_scan_codes_len; i++){
                    printf(" %04x ", window->input_scan_codes[i]);
                }
                printf("\n");
                unsigned int text_i;
                for(text_i=0;text_i<window->input_text_len;text_i++) {
                    if (window->input_text[text_i] == 'Q') {
                        close_this_window = 1;
                    }
                    if (window->input_text[text_i] == 'W')
                        spawn_new_window = 1;
                }
            }
            
            
            
            unsigned char green = 0;
            if (window->has_focus) green = 0x7f;
            unsigned char red = 0x7f;
            unsigned char alpha = 0xff;
            
            unsigned int back_color = 
                (((unsigned int)alpha)<<24) | 
                (((unsigned int)blue)<<16) | 
                (((unsigned int)green)<<8) | 
                (((unsigned int)red)<<0) ;
            
            int px, py;
            
            for (px = 0; px<window->width; px++) {
                for (py = 0; py<window->height; py++) {
                    
                    unsigned int*row   = (unsigned int*)(void*)(((unsigned char*)window->pixels) + window->pitch * py);
                    unsigned int*pixel = row + px;
                    *pixel = back_color;
                }
            }
            
            for (px = 0; px<10; px++) {
                for (py = 0; py<10; py++) {
                    
                    int sx = px + window->input_mouse_x;
                    int sy = py + window->input_mouse_y;
                    
                    if (sx <  0) continue;
                    if (sy <  0) continue;
                    if (sx >= window->width ) continue;
                    if (sy >= window->height) continue;
                    
                    unsigned int* row   = (unsigned int*)(void*)(((unsigned char*)window->pixels) + window->pitch * sy);
                    unsigned int* pixel = row + sx;
                    *pixel = 0xffffffff;
                }
            }
            
            if (close_this_window) {
                barclay_window_release(window);
                windows[window_i] = windows[--window_count];
            } else {
                barclay_window_frame_send(window);
            }
            if (spawn_new_window) {
                windows[window_count++] = barclay_window_init(0);
            }
            
            
        }
        
        for (window_i=0;window_i<window_count;window_i++) {
            barclay_window_frame_recv(windows[window_i]);
        }
        
    }
}
