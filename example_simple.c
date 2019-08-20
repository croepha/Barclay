
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
        blue++; // Change the color slightly each frame...
        
        unsigned char red   = 0x7f;
        unsigned char green = 0;
        if (window->has_focus) green = 0x7f;
        unsigned char alpha = 0xff;
        
        
        if (*window->input_text) {
            printf("New text: %s\n", window->input_text);
        }
        
        // Paint the window with a solid color
        unsigned int back_color = Barclay_PixelRGBA(red, green, blue, alpha);
        for (int px = 0; px<window->width; px++) {
            for (int py = 0; py<window->height; py++) {
                Barclay_PixelWindowXY(window, px, py) = back_color;
            }
        }
        
        barclay_window_frame_send(window, 0);
        barclay_window_frame_recv(window);
    }
}
