// Copyright 2019 David Butler <croepha@gmail.com>

#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "util_types.hpp"


struct ClientBitmapSpec {
    s32 w;
    s32 h;
    
    static const s32 bytes_per_px            = 4;
    inline s32 pitch              (){ return w * bytes_per_px; }
    inline u64 single_buffer_bytes(){ return (u64)(pitch() * h); }
    inline u64 double_buffer_bytes(){ return single_buffer_bytes() * 2; }
};


int main () {
    
    s32       bitmap_fd;
    s32       x, y, w, h;
    void*     bitmap_front = 0;
    void*     bitmap_back  = 0;
    
    
    y = 50;
    x = 50;
    w = 600; 
    h = 300;
    bitmap_fd     = -1;
    
    
    int debug_client_count = 0;
    (void)debug_client_count;
    
    
    
    size_t bitmap_size = 0;
    
    for (;;) {
        // TODO: cleanup mouse code
        
        
        h++;
        if (h > 700) {
            h = 50;
        }
        
        ClientBitmapSpec _spec = {.w = w, .h = h};
        
        
        auto _m = min_of(bitmap_front, bitmap_back);
        
        if (bitmap_size) {
            auto r20 = munmap(_m, bitmap_size);
            assert(r20 == 0);
        }
        
        if(bitmap_fd != -1) {
            auto r21 = close(bitmap_fd);
            assert(r21 == 0);
        }
        bitmap_front  = 0;
        bitmap_back   = 0;
        
        
        assert(w > 0);
        assert(h > 0);
        
        
        bitmap_size = _spec.double_buffer_bytes();
        
        
        // TODO: Look into File sealing (If we care about security at some point)
        bitmap_fd =  memfd_create("client_bitmap", MFD_CLOEXEC); 
        assert(bitmap_fd != -1);
        
        auto r14 = ftruncate(bitmap_fd, (__off_t)bitmap_size);
        assert(!r14);
        
        
        //  MAP_32BIT
        //  (void*)0x500000000000
        
        auto mapflags = MAP_SHARED;
        //  mapflags |= MAP_32BIT;
        
        auto bitmap = (u8*)mmap(0, bitmap_size, PROT_READ, mapflags, bitmap_fd, 0);
        assert(bitmap != MAP_FAILED);
        
        bitmap_front = bitmap;
        bitmap_back  = bitmap + _spec.single_buffer_bytes();
        
        
        u64 sum = 0;
        for (s32 px = 0; px<w; px++) {
            for (s32 py = 0; py<h; py++) {
                
                auto client_row   = (u32*)(void*)(((u8*)bitmap_front) + _spec.pitch() * py);
                auto client_pixel = client_row + px;
                
                auto pix_value = *client_pixel;
                
                sum += pix_value;
            }
        }
        for (s32 px = 0; px<w; px++) {
            for (s32 py = 0; py<h; py++) {
                
                auto client_row   = (u32*)(void*)(((u8*)bitmap_back) + _spec.pitch() * py);
                auto client_pixel = client_row + px;
                
                auto pix_value = *client_pixel;
                
                sum += pix_value;
            }
        }
        
        printf("bitmap_initialize, %d, %d, %p %p\n", 
               w, h,
               bitmap_front, bitmap_back);
        
        fflush(stdout);
        
    }
    
    
}
