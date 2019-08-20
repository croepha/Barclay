// Copyright 2019 David Butler <croepha@gmail.com>


#define GL_GLEXT_PROTOTYPES

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <unistd.h>
#include <sys/syscall.h>
extern "C" int memfd_create(const char *name, unsigned flags) noexcept {
    return (int)syscall(SYS_memfd_create, name, flags);
}

#include <stdio.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <sys/epoll.h>
#include <linux/netlink.h>
#include <linux/input.h>



#include <easy/profiler.h>


#include "util_types.hpp"
#include "util_lazy_io.hpp"
#include "util_DLLists.hpp"
#include "util_DynamicArray.hpp"
#include "test_protocol.hpp"


#include "util_misc.hpp"




#define NO_DEFINE_IMPLEMENTATION
//#define DEFINE_SINGLETONS

#include "util_gl.cpp"
//#include "util_lazy_sdlgl.cpp"
//#include "render_ascii_mono.cpp"
#include "render_draw_texture.cpp"
#include "render_boxes.cpp"

#undef NO_DEFINE_IMPLEMENTATION
//#undef DEFINE_SINGLETONS



#include "util_types.hpp"

#include "common.hpp"


void keymap_init();
void keymap_scancode(u16 scancode, s32 raw_value);
void window_loop();
void window_init();



struct InputDevice {
    PollType  poll_type;
    int fd;
    InputDevice* __dll_prev_next[2];
};

DLList<InputDevice> input_devices;

u32 last_poll_time = 0;
V2<s16>   screen_mouse_pos;
V2<s16>   screen_mouse_pos_last;
V2<float> screen_mouse_pos_as_float;

int uevent_socket = -1;
int server_socket = -1;
int epoll_fd = -1;

u32 frame_time_last_ms;
u16 frame_time_delta_ms;

DLList<Client> clients;


void add_input_device(char*);

V2<float> mouse_delta = {0,0};

const int scancode_input_buffer_SIZE = 8;
u16 scancode_input_buffer[scancode_input_buffer_SIZE + 1];
int scancode_input_buffer_count;

bool _debug_ignoring_events;



void ds_platform_init();
void ds_platform_frame();


u32 get_ticks_ms() {
    timespec _ts;
    auto r1 = clock_gettime(CLOCK_MONOTONIC, &_ts);
    assert(!r1);
    return (u32)(_ts.tv_sec * 1000 + _ts.tv_nsec / 1000000);
}



void poll_add(const PollType&_et, int fd) {
    epoll_event _epe = { .events=EPOLLIN|EPOLLHUP, .data={.ptr=(void*)&_et } };
    auto r15 = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &_epe);
    assert(!r15); 
}




void _debug_check_input_start();


void ds_common_init() {
    
    _debug_check_input_start();
    
    ds_platform_init();
    
    
    printf("GL_STRINGS: Vendor:%s Renderer:%s Version:%s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION)); 
    
    _gl_init();
    
    
    
    input_devices.initialize();
    keymap_init();
    window_init();
    screen_mouse_pos_as_float.x = (float)screen_mouse_pos.x;
    screen_mouse_pos_as_float.y = (float)screen_mouse_pos.y;
    
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    assert(epoll_fd != -1);
    
    
    uevent_socket = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    assert(uevent_socket != -1);
    
    sockaddr_nl uevent_socket_addr = {
        .nl_family = AF_NETLINK,
        .nl_groups = 1, // 1 for kernel only???
    };
    auto r2 = bind(uevent_socket, (sockaddr*)&uevent_socket_addr, sizeof uevent_socket_addr);
    assert(!r2);
    
    {   static const auto value_of_UEventSocket = PollType::UEventSocket;
        poll_add(value_of_UEventSocket, uevent_socket);  }
    
    
    {
        
        auto input_dir_fd = open("/sys/class/input/", O_DIRECTORY|O_RDONLY);
        assert(input_dir_fd != -1);
        auto input_dir = fdopendir(input_dir_fd);
        assert(input_dir);
        
        for (;;) {
            errno = 0;
            auto _dirent = readdir(input_dir);
            
            if (!_dirent) {
                assert(!errno);
                break;
            }
            
            if (_dirent->d_type == DT_LNK) {
                
                add_input_device(_dirent->d_name);
                
#if 0
                if (strncmp(_dirent->d_name, "event", 5) == 0) {
                    //printf("input dir:  %s \n", _dirent->d_name);
                    char _buf[1024];
                    auto _buf_len = readlinkat(input_dir_fd, _dirent->d_name, _buf, sizeof _buf-1);
                    assert(_buf_len != -1);
                    _buf[_buf_len] = 0;
                    printf("readlink:  %s \n", _buf);
                    
                }
#endif
            }
            
        }
        
        auto r10 = closedir(input_dir);
        assert(!r10);
    }
    
    server_socket = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK,  0);
    
    sockaddr_un server_socket_addr = {
        .sun_family = AF_UNIX,
        .sun_path   = SERVER_SOCKET_PATH,
    };
    
    unlink(SERVER_SOCKET_PATH);
    auto r1 = bind(server_socket, 
                   (sockaddr*)&server_socket_addr, 
                   sizeof server_socket_addr);
    assert(!r1);
    
    auto r4 = chmod(SERVER_SOCKET_PATH, 0777);
    assert(!r4);
    
    auto r3 = listen(server_socket, 8);
    assert(!r3);
    
    {   static const auto value_of_ServerConnection = PollType::ListeningSocket;
        poll_add(value_of_ServerConnection, server_socket);  }
    
    
    
    clients.initialize();
    
    int debug_client_count = 0;
    (void)debug_client_count;
    
    assert(glGetError() == GL_NO_ERROR);
    
    
    
    window_loop();
    
    
    
    
}




void bitmap_initialize(Client&client) {
    {
        ClientBitmapSpec _spec = {.w = client.size.w, .h = client.size.h};
        client.bitmap_size = client.size;
        
        assert(client.size.w > 0);
        assert(client.size.h > 0);
        
        auto bitmap_size = _spec.double_buffer_bytes();
        
        // TODO: Look into File sealing (If we care about security at some point)
        auto bitmap_fd =  memfd_create("client_bitmap", MFD_CLOEXEC); 
        assert(bitmap_fd != -1);
        
        auto r14 = ftruncate(bitmap_fd, (off_t)bitmap_size);
        assert(!r14);
        
        //  MAP_32BIT
        //  (void*)0x500000000000
        auto bitmap = (u8*)mmap(0, bitmap_size, PROT_READ, MAP_SHARED, bitmap_fd, 0);
        assert(bitmap != MAP_FAILED);
        
        client.bitmap_fd = bitmap_fd;
        client.bitmap_front = bitmap;
        client.bitmap_back  = bitmap + _spec.single_buffer_bytes();
        
        client.resized = 0;
        
    }
    
    glGenTextures(1, &client.gl_texture); 
    glBindTexture(GL_TEXTURE_2D, client.gl_texture);  
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, client.bitmap_size.w, client.bitmap_size.h, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, client.bitmap_front);
    
#if 0
    printf("bitmap_initialize, %d, %d, %p %p\n", 
           client.size.w, client.size.h,
           client.bitmap_front, client.bitmap_back);
    
    fflush(stdout);
    
#endif
    
}


void bitmap_cleanup(Client&client) {
    
    glDeleteTextures(1, &client.gl_texture);
    
    auto _m = min_of(client.bitmap_front, client.bitmap_back);
    
    ClientBitmapSpec _spec = {.w = client.bitmap_size.w, .h = client.bitmap_size.h};
    auto bitmap_size = _spec.double_buffer_bytes();
    
    auto r20 = munmap(_m, bitmap_size);
    assert(r20 == 0);
    
    auto r21 = close(client.bitmap_fd);
    assert(r21 == 0);
    
    client.bitmap_front  = 0;
    client.bitmap_back   = 0;
    client.bitmap_fd     = -1;
}




#define AwCOUNT(m_a) m_a, (sizeof m_a / sizeof m_a[0])
#define AwSIZE(m_a)  m_a, (sizeof m_a)



int _debug_main_input;
int _debug_check_input;
int _debug_check_input_epollfd;
void* _debug_check_input_thread(void*) {
    for(;;) {
        
        epoll_event _epes[16];
        auto epe_count = epoll_wait(_debug_check_input_epollfd, AwCOUNT(_epes), -1);
        assert(epe_count != -1 || errno == EINTR );
        if (epe_count == -1) {
            warn("epoll got %s\n", strerror(errno));
            continue;
        }
        
        for (int _i=0; _i<epe_count; _i++) {
            auto&_epe = _epes[_i];
            
            input_event input_events[1024];
            auto r11 = read(_epe.data.fd, AwSIZE(input_events));
            if (r11 == -1) { 
                if (errno == EWOULDBLOCK) {
                    break;
                }
                assert(0); 
            } 
            if (r11 == 0) { 
                close(_epe.data.fd);
            } 
            auto input_events_count = (u32)r11 / sizeof(input_events[0]);
            _debug_check_input += input_events_count;
        }
    }
}

void _debug_check_input_start() {
    _debug_check_input_epollfd = epoll_create1(EPOLL_CLOEXEC);
    
    pthread_t _th;
    
    auto r1 = pthread_create(&_th, 0, _debug_check_input_thread, 0);
    assert(!r1);
}
void _debug_check_input_add_device(char*dev_path) {
    auto _fd = open(dev_path, O_RDWR|O_CLOEXEC|O_NONBLOCK);
    assert(_fd != -1);
    
    epoll_event _epe = { .events=EPOLLIN|EPOLLHUP, .data={.fd=_fd} };
    auto r15 = epoll_ctl(_debug_check_input_epollfd, EPOLL_CTL_ADD, _fd, &_epe);
    assert(!r15); 
    
}


void add_input_device(char*some_path_ending_with_kernel_name) {
    
    char*kernel_name = 0;
    
    auto _last_slash = strrchr(some_path_ending_with_kernel_name, '/');
    if (_last_slash) {
        kernel_name = _last_slash + 1;
    } else {
        kernel_name = some_path_ending_with_kernel_name;
    }
    
    if (strncmp(kernel_name, "event", 5) == 0) {
        assert(strlen(kernel_name) < 16);
        assert(strlen(kernel_name) > 0);
        
        char _buf[] = "/dev/input/eventXXXXXXXXXXXXXX";
        strcpy(_buf + 11, kernel_name);
        
        _debug_check_input_add_device(_buf);
        
        
        auto _fd = open(_buf, O_RDWR|O_CLOEXEC|O_NONBLOCK);
        if (_fd != -1) {
            
            auto& _dev = *(InputDevice*)malloc(sizeof(InputDevice)) = {
                .poll_type = PollType::InputDevice,
                .fd = _fd,
            };
            input_devices.append(&_dev);
            
            poll_add(_dev.poll_type, _fd);
            
            
        } else {
            printf("Tried to open input device `%s' but failed with: %s\n",
                   _buf, strerror(errno));
        }
        
    }
}





void do_poll_event(epoll_event&_epe) {
    
    auto poll_type_from_epoll = [](epoll_event&_epe2){
        return (PollType*)_epe2.data.ptr;
    };
    
    
    
    switch (*poll_type_from_epoll(_epe)) { default: assert(0); break;
        case PollType::InputDevice: { auto&input_device=*(InputDevice*)_epe.data.ptr;
            
            if (_epe.events & EPOLLIN) {
                // read event...
                input_read_again:;
                
                input_event input_events[1024];
                auto r11 = read(input_device.fd, &input_events, sizeof input_events);
                if (r11 == -1) { 
                    if (errno == EWOULDBLOCK) {
                        goto input_read_out;
                    }
                    assert(0); 
                    goto close_input_device; 
                } 
                if (r11 == 0) { goto close_input_device; } 
                
                auto input_events_count = (u32)r11 / sizeof(input_events[0]);
                _debug_main_input += input_events_count;
                
#if 0
                static u64 _hwm;
                if (input_events_count > _hwm) _hwm = input_events_count;
                printf("input_events_count hwm :%llu\n", _hwm);
#endif
                
                assert(input_events_count*sizeof input_events[0] == (u32)r11); // make sure we dont get partial events...
                
                for (u32 i=0; i<input_events_count; i++) {
                    auto&_ie = input_events[i];
                    
                    char* _debug_type_text = 0;
                    switch (_ie.type) { default: _debug_type_text = "????"; break;
#define M(m_E) case m_E: _debug_type_text = #m_E; break;
                        M(EV_SYN) M(EV_KEY) M(EV_REL) M(EV_ABS) M(EV_MSC)
                            M(EV_SW) M(EV_LED) M(EV_SND) M(EV_REP) M(EV_FF)
                            M(EV_PWR) M(EV_FF_STATUS)
#undef M
                    }
                    
                    
                    if (_ie.type == EV_SYN) continue;
                    if (_ie.type == EV_MSC) continue;
                    if (_ie.type == EV_LED) continue; // Not sure why we would care about this... its our job to tell the keyboard what leds to have...
                    
                    if (_ie.type == EV_KEY) {
                        
                        if (!_debug_ignoring_events) {
                            keymap_scancode(_ie.code, _ie.value);
                            
                            if (_ie.value != 2 ) {
                                
                                u16 dos_scancode = _ie.code;
                                if (!_ie.value) dos_scancode += 0x8000;
                                
                                assert(scancode_input_buffer_count < scancode_input_buffer_SIZE);
                                scancode_input_buffer[scancode_input_buffer_count++] = dos_scancode;
                            }
                            
                        }
                    } else if (_ie.type == EV_ABS) {
                        if (!_debug_ignoring_events) { 
                            float _v01 = ((float)_ie.value) / ((float)(u16)-1);
                            
                            if        (_ie.code == ABS_X) {
                                screen_mouse_pos_as_float.x = _v01 * SCREEN_WIDTH;
                            } else if (_ie.code == ABS_Y) {
                                screen_mouse_pos_as_float.y = _v01 * SCREEN_HEIGHT;
                                
                            }
                        }
                    } else if (_ie.type == EV_REL) {
                        
                        
                        //printf("REL??  input_event: %s %d %d\n", _debug_type_text,  _ie.code, _ie.value);
                        
                        float _v = _ie.value;
                        
                        if (_ie.code == REL_X) {
                            mouse_delta.x += _v;
                        } else if  (_ie.code == REL_Y) {
                            mouse_delta.y += _v;
                        } else {
                            printf("REL??  input_event: %s %d %d\n", _debug_type_text,  _ie.code, _ie.value);
                        }
                        
                        
                    } else {
                        printf("WHAT??? input_event: %s %d %d\n", _debug_type_text,  _ie.code, _ie.value);
                    }
                    
                    
                }
                
                goto input_read_again;
                
            } else if (_epe.events & EPOLLHUP) {
                
                close_input_device:;
                close(input_device.fd);
            }
            
            input_read_out:;
            
        } break;
        case PollType::UEventSocket: {
            
            char buf[1024];
            
            
            memset(buf, 0, sizeof buf);
            
            struct iovec iov = { buf, sizeof(buf) };
            struct sockaddr_nl sa;
            
            struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
            auto len = recvmsg(uevent_socket, &msg, 0);
            
            // printf("GOT NETLINK PACKET??? %d %x %ld %zu %zu\n", sa.nl_pid, sa.nl_groups,len, iov.iov_len, msg.msg_controllen);
            
            
            char* _devpath   = 0;
            char* _subsystem = 0;
            
            char* _s = buf;
            
            for (;;){
                if (_s >= buf + len) break;
                
                if (strncmp(_s, "DEVPATH", 7) == 0) {
                    _s = _s + 8;
                    _devpath = _s;
                } else if (strncmp(_s, "SUBSYSTEM", 9) == 0) {
                    _s = _s + 10;
                    _subsystem = _s;
                }
                
                if (_s >= buf + len) break;
                
                _s = (char*)memchr(_s, 0, size_t(len-(_s-buf)) );
                if (!_s) {
                    assert(0);
                    goto uevent_fail;
                }
                _s++;
            }
            
            
            if (_subsystem && strcmp(_subsystem, "input") == 0 && _devpath) {
                //printf("input: devpath: %s \n", _devpath);
                add_input_device(_devpath);
            }
            
            uevent_fail:;
            
            
            
        } break;
        
        case PollType::ListeningSocket: {
            auto client_socket_fd = accept4(server_socket, 0, 0, 
                                            SOCK_NONBLOCK|SOCK_CLOEXEC);
            assert(client_socket_fd != -1);
            
            auto& new_client = *(Client*)malloc(sizeof (Client)) = {
                ._poll_type = PollType::ClientData,
                .fd = client_socket_fd,
                .pos = { 50, 50},
                .size = { 600, 300 },
                
            };
            
            clients.append(&new_client);
            poll_add(new_client._poll_type, client_socket_fd);
            
            bitmap_initialize(new_client);
            
            {
                auto&client = new_client;
                Packet_Size _p = {.size = client.size };
                client.bitmap_size = client.size;
                auto r22 = write(client.fd, &_p, sizeof _p);
                assert(r22 == sizeof _p);
                
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
                    
                    auto fdptr = (int *) (void*) CMSG_DATA(&cmsg);
                    fdptr[0] = client.bitmap_fd;
                    
                    auto r13 = sendmsg(client.fd, &msg, 0);
                    assert(r13 == 0);
                }
            }
            
            
            
        } break;
        case PollType::ClientData: {
            auto& client = *(Client*)(void*)poll_type_from_epoll(_epe);
            
            assert(_epe.events & EPOLLIN);
            
            ClientToServerPacket packet;
            auto r11 = read(client.fd, &packet, sizeof packet);
            
            if ((r11 == -1 && errno == ECONNRESET) ||
                r11 == 0) {
                
                printf("Freeing client\n");
                
                bitmap_cleanup(client);
                
                free(client.buffered_text_input_start);
                
                auto r22 = close(client.fd);
                assert(r22 == 0);
                
                clients.remove(&client);
                free(&client);
                
            } else {
                assert(r11 == sizeof packet);
                
                client.wait_for_input = packet.wait_for_input;
                
                swap(client.bitmap_front, client.bitmap_back);
                
                //assert(!client.is_flipping);
                // TODO: Maybe shouldn't do the texture copy until were actually ready to 
                //   render, misbehaving clients could be sending multiple frames....
                
                
                glBindTexture(GL_TEXTURE_2D, client.gl_texture);  
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0,
                                client.bitmap_size.w, client.bitmap_size.h,
                                GL_RGBA, GL_UNSIGNED_BYTE, client.bitmap_front);
                glGenerateMipmap(GL_TEXTURE_2D);
                
                
                client.is_flipping = 1;
                //printf("flip start\n");
                
            }
            
            
        } break;
    }
    
}

void server_frame_io() {
    EASY_FUNCTION();
    
    _debug_main_input = 0;
    
    scancode_input_buffer_count = 0;
    
    screen_mouse_pos_last = screen_mouse_pos;
    mouse_delta = {0, 0};
    
    
#if 0    
    // TODO: This assumes that all of last_poll_time was waiting, but its not...
    //  but only a small fraction of poll_time isn't wait time...
    //  some of the flip time isn't waiting either... thats what the .90 is for
    
    
    auto non_wait_time_for_last_frame = sdlgl.frame_time_last_ms -
        sdlgl.swap_time_last_ms - last_poll_time;
    auto last_wait_time = sdlgl.frame_time_last_ms - non_wait_time_for_last_frame;
    
    auto poll_target_time_ms = (u32)(last_wait_time * .9f);
    
#endif
    
    
    auto poll_target_time_ms = (u32)10;  // This is hard coded for 60FPS?
    
    auto poll_start_time_ms = get_ticks_ms();
    
    
    for (;;) {
        
        auto poll_time_so_far_ms = get_ticks_ms() - poll_start_time_ms;
        if (poll_time_so_far_ms >= poll_target_time_ms) {
            last_poll_time = poll_time_so_far_ms;
            break;
        }
        auto poll_time_left = poll_target_time_ms - poll_time_so_far_ms;
        
        epoll_event _epe[16];
        auto epe_count = epoll_wait(epoll_fd, AwCOUNT(_epe), (s32)poll_time_left);
        assert(epe_count != -1);
        
        for (int _i=0; _i<epe_count; _i++) {
            do_poll_event(_epe[_i]);
        }
#if 0
        if (epe_count) {
            do_poll_event(_epe);
        }  // todo? we could probably just break on the else case...
#endif
    }
    
    
    auto prev_check_input = __sync_lock_test_and_set(&_debug_check_input, 0);
    
    static int  _sum_diff;
    
    _sum_diff += _debug_main_input - prev_check_input;
    
    //printf("Event count:%6d %6d %6d\n", _debug_main_input, prev_check_input, _sum_diff);
    
    if (_debug_ignoring_events) { 
        mouse_delta = {0, 0};
    }
    
    screen_mouse_pos_as_float = screen_mouse_pos_as_float + mouse_delta;
    
    if (screen_mouse_pos_as_float.x <  0)
        screen_mouse_pos_as_float.x =  0; 
    if (screen_mouse_pos_as_float.x > SCREEN_WIDTH-1)
        screen_mouse_pos_as_float.x = SCREEN_WIDTH-1;
    if (screen_mouse_pos_as_float.y < 0)
        screen_mouse_pos_as_float.y = 0; 
    if (screen_mouse_pos_as_float.y > SCREEN_HEIGHT-1)
        screen_mouse_pos_as_float.y = SCREEN_HEIGHT-1;
    
    screen_mouse_pos = {
        (s16)screen_mouse_pos_as_float.x,
        (s16)screen_mouse_pos_as_float.y
    };
    
}


void server_frame_update_clients() {
    
    for(auto&client:clients) {
        
        {   // copy new text input into client buffer....
            //  todo should we really just have a hard limit here???
            
            
            u64 new_needed_space = client.new_text_input.len + client.buffered_text_input_len + 1 ;
            assert(new_needed_space < (u32)-1);
            
            if (new_needed_space > client.buffered_text_input_size) {
                client.buffered_text_input_size = (u32)new_needed_space * 2;
                
                assert(client.buffered_text_input_size > new_needed_space); // overflow check
                
                client.buffered_text_input_start = 
                    (char*)realloc(
                    client.buffered_text_input_start, 
                    client.buffered_text_input_size);
            }
            if (client.new_text_input.len) {
                memcpy(client.buffered_text_input_start +
                       client.buffered_text_input_len,
                       client.new_text_input.start,
                       client.new_text_input.len);
            }
            
            client.buffered_text_input_len = (u32)new_needed_space - 1;
            *(client.buffered_text_input_start + client.buffered_text_input_len) = 0;
            
        }
        
        {   // like above, but scanncodes this time...
            
            u64 new_needed_space = client.new_scancode_input.len + client.buffered_scancode_input_len + 2 ;
            assert(new_needed_space < (u32)-1);
            
            if (new_needed_space > client.buffered_scancode_input_size) {
                client.buffered_scancode_input_size = (u32)new_needed_space * 2;
                
                assert(client.buffered_scancode_input_size > new_needed_space); // overflow check
                
                client.buffered_scancode_input_start = 
                    (char*)realloc(
                    client.buffered_scancode_input_start, 
                    client.buffered_scancode_input_size);
            }
            if (client.new_scancode_input.len) {
                memcpy(client.buffered_scancode_input_start +
                       client.buffered_scancode_input_len,
                       client.new_scancode_input.start,
                       client.new_scancode_input.len);
            }
            
            client.buffered_scancode_input_len = (u32)new_needed_space - 2;
            
            *(client.buffered_scancode_input_start + client.buffered_scancode_input_len + 0) = 0;
            *(client.buffered_scancode_input_start + client.buffered_scancode_input_len + 1) = 0;
            
            
        }
        
        bool has_input = 0;
        
        if (client.resized) has_input = 1;
        if (client.buffered_scancode_input_len) has_input = 1;
        if (client.buffered_text_input_len) has_input = 1;
        
        
        
        if (client.is_flipping && 
            (!client.wait_for_input || has_input )
            ) {
            
            const int _p_buf_SIZE = 1024;
            const int _fd_buf_SIZE = 8;
            
            u8  _p_buf[_p_buf_SIZE];
            const int _c_buf_size = CMSG_SPACE(sizeof(int) * _fd_buf_SIZE);
            static_assert(_c_buf_size >=  sizeof(cmsghdr) + sizeof(int) * _fd_buf_SIZE);
            alignas(alignof(cmsghdr)) u8 _c_buf[_c_buf_size];
            
            auto&_chdr = *(cmsghdr*)(void*)_c_buf;
            auto _fd_buf  = (int*)(void*)(_c_buf + sizeof _chdr);
            assert((void*)_fd_buf == (void*)CMSG_DATA(&_chdr));
            
            
            auto _p_next = _p_buf;
            auto _fd_next = _fd_buf;
            
            auto& _p_header = *(Packet_Header*)(void*)_p_next = {
                .version    = CLIENT_PROTOCOL_VERSION,
                .mouse_pos  = client.mouse_pos,
                .did_resize = client.resized,
                .has_focus  = client.has_focus,
            };
            _p_next += sizeof _p_header;
            assert(_p_next - _p_buf <= _p_buf_SIZE); 
            
            if (client.resized) {
                
                bitmap_cleanup(client);
                bitmap_initialize(client);
                
                auto&_p_resize = *(Packet_Size*)(void*)_p_next = {
                    .size = client.size,
                };
                _p_next += sizeof _p_resize;
                *_fd_next++ = client.bitmap_fd;
                
                assert(_p_next  - _p_buf   < _p_buf_SIZE); 
                assert(_fd_next - _fd_buf <= _fd_buf_SIZE);
                
            }
            
            
            {
                // Send scancodes
                s64 bytes_to_send = _p_buf_SIZE - (_p_next - _p_buf); // space left
                assert(bytes_to_send >= 0);
                assert(bytes_to_send < (u32)-1);
                bytes_to_send = (bytes_to_send / 2) * 2; // Round to even amount...
                
                
                assert(client.buffered_scancode_input_len != 1);
                assert(bytes_to_send != 1);
                
                set_min(bytes_to_send, client.buffered_scancode_input_len);
                
                _p_header.scancode_len   =  (u32)bytes_to_send;
                
                memcpy(_p_next, client.buffered_scancode_input_start, (u64)bytes_to_send);
                memmove(client.buffered_scancode_input_start, 
                        client.buffered_scancode_input_start + bytes_to_send,
                        client.buffered_scancode_input_len - (u32)bytes_to_send
                        );
                client.buffered_scancode_input_len -= bytes_to_send;
                *(client.buffered_scancode_input_start + client.buffered_scancode_input_len + 0) = 0;
                *(client.buffered_scancode_input_start + client.buffered_scancode_input_len + 1) = 0;
                
                _p_next += bytes_to_send;
                
                
            }
            
            
            
            
            {
                // Send text input...
                s64 bytes_to_send = _p_buf_SIZE - (_p_next - _p_buf); // space left
                assert(bytes_to_send >= 0);
                assert(bytes_to_send < (u32)-1);
                
                set_min(bytes_to_send, client.buffered_text_input_len);
                
                _p_header.text_len   = client.buffered_text_input_len;
                
                
                // TODO: If we are doing unicode, then we should probably consider
                //   not spltting the final character if its multibyte...
                memcpy(_p_next, client.buffered_text_input_start, (u64)bytes_to_send);
                memmove(client.buffered_text_input_start, 
                        client.buffered_text_input_start + bytes_to_send,
                        client.buffered_text_input_len - (u32)bytes_to_send
                        );
                client.buffered_text_input_len -= bytes_to_send;
                *(client.buffered_text_input_start + client.buffered_text_input_len) = 0;
                
                _p_next += bytes_to_send;
                
                
            }
            
            
            iovec _iov = {
                .iov_base = _p_buf,
                .iov_len  = (size_t)(_p_next - _p_buf),
            };
            
            _chdr = {
                .cmsg_level = SOL_SOCKET,
                .cmsg_type = SCM_RIGHTS,
                .cmsg_len = CMSG_LEN(sizeof(int) * (size_t)(_fd_next - _fd_buf)),
            };
            
            msghdr msg = {
                .msg_iov = &_iov,
                .msg_iovlen = 1,
                .msg_control = _c_buf,
                .msg_controllen = CMSG_SPACE(sizeof(int) * (size_t)(_fd_next - _fd_buf)),
            };
            
            auto r13 = sendmsg(client.fd, &msg, 0);
            if (r13 == -1) {
                assert(errno == EPIPE);
                // probably client is dead, should be cought in the main epoll loop...
                
            } else { 
                assert(r13 != -1);
                assert(r13 == (long)_iov.iov_len);
                
                //printf("after flip packet sent \n");
                
            }
            client.is_flipping = 0;
        }
    }
    
    
}

void ds_common_frame() {
    EASY_FUNCTION();
    
    
    ds_platform_frame();
    
    glClear(GL_COLOR_BUFFER_BIT);
    assert(glGetError() == GL_NO_ERROR);
    
    {
        auto frame_time_ms = get_ticks_ms();
        frame_time_delta_ms = (u16)(frame_time_ms - frame_time_last_ms);
        frame_time_last_ms = frame_time_ms;
        
        //printf("frame_time_delta: %u\n", frame_time_delta_ms);
    }
    
    
    
    
}




int main () {
    EASY_MAIN_THREAD;
    profiler::startListen();
    frame_time_last_ms = get_ticks_ms();
    ds_common_init();
}

