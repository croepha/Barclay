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

int main () {
    
    struct BarclayWindow*window = barclay_window_init(0);
    unsigned char blue  = 0x00;
    
    for (;;) {
        
        unsigned char red   = 0x7f;
        unsigned char green = 0;
        if (window->has_focus) green = 0x7f;
        unsigned char alpha = 0xff;
        
        // Print out any text that has been entered
        char needs_nl = 0;
        if (window->input_text_len) {
            printf("New text: `%s' ", window->input_text);
            needs_nl = 1;
        }
        if (window->input_scan_codes_len) {
            printf("Raw Scancodes: ");
            for (unsigned int i=0;i<window->input_scan_codes_len; i++){
                printf(" %04x ",window->input_scan_codes[i]);
            }
            needs_nl = 1;
        }
        if(needs_nl) printf("\n");
        
        
        // Paint the window with a solid color
        unsigned int back_color = Barclay_PixelRGBA(red, green, blue, alpha);
        for (int px = 0; px<window->width; px++) {
            for (int py = 0; py<window->height; py++) {
                Barclay_PixelWindowXY(window, px, py) = back_color;
            }
        }
        
        // Render a white box where the mouse is inside of our window
        for (int px = 0; px<10; px++) {
            for (int py = 0; py<10; py++) {
                int sx = px + window->input_mouse_x;
                int sy = py + window->input_mouse_y;
                if (sx <  0) continue;
                if (sy <  0) continue;
                if (sx >= window->width ) continue;
                if (sy >= window->height) continue;
                Barclay_PixelWindowXY(window, sx, sy) = 0xffffffff;
            }
        }
        
        barclay_window_frame_send(window, 0);
        barclay_window_frame_recv(window);
        
    }
}

