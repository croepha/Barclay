// Copyright 2019 David Butler <croepha@gmail.com>




#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "util_types.hpp"

#include "test_protocol.hpp"


ClientBitmapSpec bitmap_spec;
int bitmap_fd;
int socket_fd;
u8* bitmap_front;
u8* bitmap_back;



void bitmap_initialize() {
    
    
    //printf("bitmap_initialize %d %d %d\n", bitmap_fd, bitmap_spec.w, bitmap_spec.h);
    
    assert(bitmap_fd != -1);
    
    
    
    bitmap_front = (u8*)mmap(0, (size_t)bitmap_spec.double_buffer_bytes(), PROT_READ|PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    bitmap_back  = bitmap_front + bitmap_spec.single_buffer_bytes();
    
    
}

void bitmap_cleanup() {
    
    auto _m = min_of(bitmap_front, bitmap_back);
    
    auto r20 = munmap(_m, (size_t)bitmap_spec.double_buffer_bytes());
    assert(r20 == 0);
    
    auto r21 = close(bitmap_fd);
    assert(r21 == 0);
    
    
}



int main () {
    
    
    socket_fd = ::socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC,  0);
    
    
    sockaddr_un socket_addr = {
        .sun_family = AF_UNIX,
        .sun_path   = SERVER_SOCKET_PATH,
    };
    
    
    auto r1 = connect(socket_fd, 
                      (sockaddr*)&socket_addr, 
                      sizeof socket_addr);
    assert(!r1);
    
    
    
    {
        Packet_Size _p;
        auto r22 = read(socket_fd, &_p, sizeof _p);
        assert(r22 == sizeof _p);
        bitmap_spec = {.w=_p.size.w, .h=_p.size.h };
    }
    
    
    
    
    { 
        
        static const int NUM_FD = 1;
        
        alignas(alignof(cmsghdr)) char buf[CMSG_SPACE(sizeof(int) * NUM_FD)];
        struct msghdr msg = { 
            .msg_control = buf,
            .msg_controllen = sizeof(buf),
        };
        
        auto&cmsg = *CMSG_FIRSTHDR(&msg) = {
            .cmsg_level = SOL_SOCKET,
            .cmsg_type = SCM_RIGHTS,
            .cmsg_len = CMSG_LEN(sizeof(int) * NUM_FD),
        };
        
        auto r2 = recvmsg(socket_fd, &msg, 0);
        assert(r2 == 0 );
        
        auto fdptr = (int *) (void*) CMSG_DATA(&cmsg);
        bitmap_fd = fdptr[0]; 
        
    }
    
    bitmap_initialize();
    
    bool has_focus = 0;
    
    V2<s16> mouse_pos = {};
    
    u8 blue = 0;
    for (;;) {
        
        
        u8 green = 0;
        if (has_focus) green = 0xff;
        u8 red = 0xff;
        u8 alpha = 0xff;
        
        u32 back_color = (((u32)alpha)<<24) | (((u32)blue)<<16) | (((u32)green)<<8) | (((u32)red)<<0) ;
        //printf("red: %d %x %x \n", red, back_color, (((u32)red)<<24));
        
        for (int px = 0; px<bitmap_spec.w; px++) {
            for (int py = 0; py<bitmap_spec.h; py++) {
                
                auto row   = (u32*)(void*)(((u8*)bitmap_back) + bitmap_spec.pitch() * py);
                auto pixel = row + px;
                *pixel = back_color;
            }
        }
        
        for (int px = 0; px<10; px++) {
            for (int py = 0; py<10; py++) {
                
                auto sx = px + mouse_pos.x;
                auto sy = py + mouse_pos.y;
                
                if (sx <  0) continue;
                if (sy <  0) continue;
                if (sx >= bitmap_spec.w) continue;
                if (sy >= bitmap_spec.h) continue;
                
                auto row   = (u32*)(void*)(((u8*)bitmap_back) + bitmap_spec.pitch() * sy);
                auto pixel = row + sx;
                *pixel = 0xffffffff;
            }
        }
        
        blue++;
        
        
        
        {
            Packet _p;
            
            strcpy(_p.text, "___TEST!!");
            _p.text[1] = '0' + 1;
            
            
            swap(bitmap_front, bitmap_back);
            
            auto r11 = write(socket_fd, &_p, sizeof _p);
            assert(r11 == sizeof _p);
            
            
        }
        
        
        const int _p_buf_SIZE = 1024;
        const int _fd_buf_SIZE = 8;
        
        u8  _p_buf[_p_buf_SIZE];
        const int _c_buf_size = CMSG_SPACE(sizeof(int) * _fd_buf_SIZE);
        static_assert(_c_buf_size >=  sizeof(cmsghdr) + sizeof(int) * _fd_buf_SIZE);
        alignas(alignof(cmsghdr)) u8 _c_buf[_c_buf_size];
        
        
        iovec _iov = {
            .iov_base = _p_buf,
            .iov_len  = _p_buf_SIZE,
        };
        auto&_chdr = *(cmsghdr*)(void*)_c_buf = {
            .cmsg_level = SOL_SOCKET,
            .cmsg_type = SCM_RIGHTS,
            .cmsg_len = CMSG_LEN(sizeof(int) * _fd_buf_SIZE),
        };
        msghdr msg = {
            .msg_iov = &_iov,
            .msg_iovlen = 1,
            .msg_control = _c_buf,
            .msg_controllen = CMSG_SPACE(sizeof(int) * _fd_buf_SIZE),
        };
        
        auto _p_buf_len = recvmsg(socket_fd, &msg, MSG_CMSG_CLOEXEC);
        assert(_p_buf_len != -1 );
        
        
        auto _fd_buf  = (int*)(void*)(_c_buf + sizeof _chdr);
        assert((void*)_fd_buf == (void*)CMSG_DATA(&_chdr));
        auto _fd_buf_len = (int*)(_c_buf + _chdr.cmsg_len) - _fd_buf;
        
        auto _p_next = _p_buf;
        auto _fd_next = _fd_buf;
        
        auto& _p_header = *(Packet_Header*)_p_next;
        _p_next += sizeof _p_header;
        assert(_p_next - _p_buf <= _p_buf_len);
        
        assert(_p_header.version == CLIENT_PROTOCOL_VERSION);
        
        mouse_pos = _p_header.mouse_pos;
        has_focus = _p_header.has_focus;
        
        if (_p_header.did_resize) {
            bitmap_cleanup();
            
            auto&_p_resize = *(Packet_Size*)_p_next;
            _p_next += sizeof _p_resize;
            bitmap_fd = *_fd_next++;
            assert(_p_next  - _p_buf  <= _p_buf_len); 
            assert(_fd_next - _fd_buf <= _fd_buf_len);
            
            bitmap_spec = {.w=_p_resize.size.w, .h=_p_resize.size.h };
            
            bitmap_initialize();
            
        }
        
        auto text_start = (char*)_p_next;
        _p_next += _p_header.text_len;
        auto scancode_start = (u16*)_p_next;
        auto scancode_count = _p_header.scancode_len /2;
        
        
        if (_p_header.text_len || scancode_count) {
            printf("got text: %d `%.*s'  scanncodes: %d: ",
                   _p_header.text_len,
                   _p_header.text_len, text_start,
                   scancode_count);
            for (u32  i=0;i<scancode_count; i++){
                printf(" %04x ",scancode_start[i]);
            }
            printf("\n");
            
        }
        
        
        
    }
    
    
    
    
}
