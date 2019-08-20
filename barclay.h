
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

#ifdef __cplusplus
#define BARCLAY_DECL extern "C"
#else
#define BARCLAY_DECL
#endif


struct BarclayWindow {
    void          *pixels;
    char           has_focus;
    unsigned short width;
    unsigned short height;
    unsigned short pitch;
    short          input_mouse_x;
    short          input_mouse_y;
    unsigned short*input_scan_codes;
    unsigned int   input_scan_codes_len;
    char          *input_text;
    unsigned int   input_text_len;
    int            socket_fd;
};

BARCLAY_DECL struct BarclayWindow*barclay_window_init(char*config);
BARCLAY_DECL void barclay_window_frame_send(struct BarclayWindow*, char wait_for_input);
BARCLAY_DECL void barclay_window_frame_recv(struct BarclayWindow*);
BARCLAY_DECL void barclay_window_release(struct BarclayWindow*);
BARCLAY_DECL void barclay_window_opengl_make_current(struct BarclayWindow*);
#undef BARCLAY_DECL

/*
// TODO
#define BARCLAY_SCANCODE_UP 0x9323
*/

/*

call barclay_window_frame_send to tell the display server that we are done
rendering to the frame and we are ready to flip

The wait_for_input parameter is used to control whether or not we want to draw
every frame or if we want to wait for any input.  This will control how long 
barclay_window_frame_recv will block.  For example, if you had a tool that you 
wanted only needed to draw whenever the user pressed a key, then you could set
wait_for_input=1 and then when you call barclay_window_frame_recv, it will
block until there is actual input, essentially putting your application to
sleep briefly.  However if you had a multimedia application or simply had
animations that needed to be updated each frame, regardless of any input then
you should set wait_for_input=0 and then barclay_window_frame_recv will only
block until the rendered frame is visible to the user.

call barclay_window_frame_recv to wait for the flip and also to recv any new
input.

input_scan_codes is the raw scancodes given by the lower level
operating system.  They closely match the IBM PC/MSDOS scancodes, and you can
get a list of possible values from the input.h header from the linux source
tree.  I would have provided an equivilent list here, but since the 
variablility in physical keyboard layouts basically makes the list un-reliable
I have omitted it.  To properly use scancodes, your application should prompt
for the scancodes and save the values and then use that value in the future.
That is the only reliable way to give symbolic meaning to the scancode value.

if (scancode&0x8000) then its a keydown, otherwise its a keyup

TODO: actually, I should include some specific scancodes that are reliable, like
mouse clicks]

The display server will do some work to abstract way some of the physical
keyboard variance but does it at a high level, via input_text.  input_text is
a UTF-8 stream of characters that represent the text that the user typed. 
input_text also includes some control caracters that have special user defined
meanings.


*/

// Here is some high level stuff...

#define Barclay_PixelRGBA(m_red,m_green,m_blue,m_alpha) \
((((unsigned int)(m_alpha))<<24) | \
(((unsigned int)(m_blue ))<<16) | \
(((unsigned int)(m_green))<< 8) | \
(((unsigned int)(m_red  ))<< 0) ) 

#define Barclay_PixelWindowXY(m_window, m_x, m_y) \
*((unsigned int*)(void*)( \
((unsigned char*)(m_window)->pixels) + \
(m_window)->pitch * (m_y) + (m_x)*4))







