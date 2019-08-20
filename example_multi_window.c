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

#define MAX_COUNT(m_array) (sizeof(m_array)/(sizeof(m_array)[0]))

struct BarclayWindow* windows[128];
unsigned int   window_count;

void spawn_window() {
    if (window_count >= MAX_COUNT(windows)) {
        printf("too many windows, not opening another one...\n");
    } else {
        windows[window_count++] = barclay_window_init(0);
    }
}

int main () {
    
    spawn_window();
    
    unsigned char blue  = 0x00;
    while(window_count) {
        blue++; // Change the color slightly each frame...
        
        for (unsigned int window_i=0;window_i<window_count;) {
            struct BarclayWindow*window = windows[window_i];
            
            unsigned char red   = 0x7f;
            unsigned char green = 0;
            if (window->has_focus) green = 0x7f;
            unsigned char alpha = 0xff;
            
            
            char should_close = 0;
            for(unsigned int i=0;i<window->input_text_len;i++) {
                // If the user types 'W' then we open another window
                if (window->input_text[i] == 'W') {
                    spawn_window();
                }
                // If the user types 'Q' then we close this window
                if (window->input_text[i] == 'Q') {
                    should_close = 1;
                    
                }
            }
            
            if (should_close) {
                barclay_window_release(window);
                windows[window_i] = windows[--window_count];
            } else {
                // Paint the window with a solid color
                unsigned int back_color = Barclay_PixelRGBA(red, green, blue, alpha);
                for (int px = 0; px<window->width; px++) {
                    for (int py = 0; py<window->height; py++) {
                        Barclay_PixelWindowXY(window, px, py) = back_color;
                    }
                }
                
                barclay_window_frame_send(window, 0);
                window_i++;
            }
        }
        
        for (unsigned int window_i=0;window_i<window_count;window_i++) {
            struct BarclayWindow*window = windows[window_i];
            barclay_window_frame_recv(window);
        }
        
    }
}

