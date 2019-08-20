// Copyright 2019 David Butler <croepha@gmail.com>


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "util_types.hpp"

#include <errno.h>

#include "common.hpp"
#include "test_protocol.hpp"

#include "barclay.h"

#include <GL/gl.h>
#include <GL/glext.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>


static void egl_debug_proc(
EGLenum error,
const char *command,
EGLint messageType,
EGLLabelKHR threadLabel,
EGLLabelKHR objectLabel,
const char* message) {
    
    printf("EGL_DEBUG_PROC: %d %s %d %p %p %s\n",
           error, command, messageType, threadLabel, objectLabel, message
           );
    
    assert(0);
}


char null_value[2] = {0,0};

static const int _p_buf_SIZE = 1024;
static const int _fd_buf_SIZE = 8;
static const int _c_buf_size = CMSG_SPACE(sizeof(int) * _fd_buf_SIZE);
static_assert(_c_buf_size >=  sizeof(cmsghdr) + sizeof(int) * _fd_buf_SIZE);


struct WindowPrivate {
    BarclayWindow p;
    int bitmap_fd;
    void* bitmap_front;
    EGLSurface egl_surface;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLConfig  egl_config;
    bool waiting_for_flip;
    
    alignas(4)               u8  _p_buf[_p_buf_SIZE];
    alignas(alignof(cmsghdr)) u8 _c_buf[_c_buf_size];
    
};




static void bitmap_initialize(WindowPrivate&window) {
    
    
    ClientBitmapSpec bitmap_spec = { window.p.width, window.p.height };
    window.p.pitch = (u16)bitmap_spec.pitch();
    assert(window.bitmap_fd != -1);
    window.bitmap_front = (u8*)mmap(0, (size_t)bitmap_spec.double_buffer_bytes(), PROT_READ|PROT_WRITE, MAP_SHARED, window.bitmap_fd, 0);
    window.p.pixels  = (u8*)window.bitmap_front + bitmap_spec.single_buffer_bytes();
    
    if (window.egl_display) {
        
        EGLint surface_attributes[] = {
            EGL_HEIGHT, window.p.height,
            EGL_WIDTH , window.p.width,
            EGL_NONE,
        };
        
        window.egl_surface = eglCreatePbufferSurface(
            window.egl_display, window.egl_config, surface_attributes);
        assert(window.egl_surface);
        
    }
    
    
}

static void bitmap_cleanup(WindowPrivate&window) {
    
    if (window.egl_surface) {
        eglDestroySurface(window.egl_display, window.egl_surface);
    }
    
    auto _m = min_of(window.bitmap_front, window.p.pixels);
    
    ClientBitmapSpec bitmap_spec = { window.p.width, window.p.height };
    auto r20 = munmap(_m, (size_t)bitmap_spec.double_buffer_bytes());
    assert(r20 == 0);
    
    auto r21 = close(window.bitmap_fd);
    assert(r21 == 0);
    
}

#if 0
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#endif


extern "C" BarclayWindow*barclay_window_init(char* config_string_) {
    
    EGLenum gl_api = 0;
    s32     gl_major = 0;
    s32     gl_minor = 0;
    bool    gl_set_debug = 0;
    s32     gl_msaa = 0;
    s32     gl_depth_bits = 0;
    
    // TODO: super lazy today...
    
    // TODO: The debug fatal warnings should probably provide additional context
    //    specifically we should print out the name of this function and maybe the
    //    init config string...
    
    char config_string_space[1024];
    char*config_string = 0;
    if (config_string_) {
        strncpy(config_string_space, config_string_, sizeof config_string_space -1);
        config_string_space[sizeof config_string_space -1] = 0;
        config_string = config_string_space;
    }
    
    while(config_string) {
        
        char* word = config_string;
        config_string = strchr(config_string, ' ');
        if(config_string) {
            *config_string = 0;
            config_string++;
        }
        
        
        auto colon = strchr(word, ':');
        if (colon) {
            *colon = 0;
        }
        
        if (!gl_api) {
            if (strcmp(word, "gles")==0) {
                gl_api = EGL_OPENGL_ES_API;
            } else if (strcmp(word, "opengl")==0) {
                gl_api = EGL_OPENGL_API;
            } else { 
                fatal("We were looking for `gles' or `opengl'");
            }
            
            
            if (!colon) {
                fatal("We were looking for a colon...");
            }
            auto dot = strchr(colon+1, '.');
            if (!dot) {
                fatal("We were looking for a dot...");
            }
            *dot = 0;
            
            errno = 0;
            gl_major = (s32)strtol(colon+1, 0, 10);
            gl_minor = (s32)strtol(dot+1  , 0, 10);
            assert(errno == 0);
            
        } else {
            
            if (strcmp(word, "debug")==0) {
                gl_set_debug = 1;
            }
            
            if (strcmp(word, "msaa")==0) {
                if (!colon) {
                    fatal("We were looking for a colon...");
                }
                gl_msaa = (s32)strtol(colon+1, 0, 10);
            }
            
            if (strcmp(word, "depth")==0) {
                gl_depth_bits = 8;
            }
        }
        
    }
    
    
    
    
    
    
    
    
    
    auto&window = *(WindowPrivate*)malloc(sizeof(WindowPrivate)) = {};
    window.p.input_text       = (char*)null_value;
    window.p.input_scan_codes = (u16*)(void*)null_value;
    
    window.p.socket_fd = ::socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC,  0);
    
    sockaddr_un socket_addr = {
        .sun_family = AF_UNIX,
        .sun_path   = SERVER_SOCKET_PATH,
    };
    
    
    auto r1 = connect(window.p.socket_fd, 
                      (sockaddr*)&socket_addr, 
                      sizeof socket_addr);
    assert(!r1);
    
    
    
    {
        Packet_Size _p;
        auto r22 = read(window.p.socket_fd, &_p, sizeof _p);
        assert(r22 == sizeof _p);
        window.p.width  = (u16)_p.size.w;
        window.p.height = (u16)_p.size.h;
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
        
        auto r2 = recvmsg(window.p.socket_fd, &msg, 0);
        assert(r2 == 0 );
        
        auto fdptr = (int *) (void*) CMSG_DATA(&cmsg);
        window.bitmap_fd = fdptr[0]; 
        
    }
    
    if (gl_api == EGL_OPENGL_API || gl_api == EGL_OPENGL_ES_API) {
        
#if 0
        // TODO? We will probably need this on nvidia when X11 is not running?? 
        auto eglQueryDevicesEXT_ = (typeof(&eglQueryDevicesEXT))eglGetProcAddress("eglQueryDevicesEXT");
        
        EGLDeviceEXT devices[10];
        int device_count;
        eglQueryDevicesEXT_(10, devices, &device_count);
        
        printf("EGL device count: %d \n", device_count);
        
#endif
        
#if 0   
        auto drm_fd = open("/dev/dri/renderD128", O_RDWR);
        assert(drm_fd != -1);
        
        auto resources = drmModeGetResources(drm_fd);
        if (!resources) {
            printf("drmModeGetResources failed: %s\n", strerror(errno));
            assert(0);
        }
        auto gbm_dev = gbm_create_device(drm_fd);
        
        auto eglGetPlatformDisplayEXT_ = (typeof(&eglGetPlatformDisplayEXT))eglGetProcAddress("eglGetPlatformDisplayEXT");
        window.egl_display = eglGetPlatformDisplayEXT_(EGL_PLATFORM_GBM_KHR, gbm_dev, NULL);
        
#endif
        
#if 1
        window.egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
        
        
        
        
        
        EGLint config_attributes [10]; 
        EGLint context_attributes[10]; 
        
        {
            auto config_attributes_next  = config_attributes;
            auto context_attributes_next = context_attributes;
            
#define _append(m_a, m_v) \
            assert(m_a ## _next < m_a + sizeof m_a/sizeof m_a[0]); \
            *m_a ## _next++ = m_v
            
            _append(config_attributes, EGL_SURFACE_TYPE);
            _append(config_attributes, EGL_PBUFFER_BIT);
            
            
            
            if (gl_depth_bits) {
                _append(config_attributes, EGL_DEPTH_SIZE);
                _append(config_attributes, gl_depth_bits);
            }
            
            if (gl_msaa) {
                _append(config_attributes, EGL_SAMPLE_BUFFERS);
                _append(config_attributes, 1);
                _append(config_attributes, EGL_SAMPLES);
                _append(config_attributes, 4);
            }
            
            _append(config_attributes, EGL_NONE);
            
            _append(context_attributes, EGL_CONTEXT_MAJOR_VERSION_KHR);
            _append(context_attributes, gl_major);
            _append(context_attributes, EGL_CONTEXT_MINOR_VERSION_KHR);
            _append(context_attributes, gl_minor);
            
            if (gl_api == EGL_OPENGL_API) {
                _append(context_attributes, EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
                _append(context_attributes, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
            }
            
            if (gl_set_debug) {
                _append(context_attributes, EGL_CONTEXT_FLAGS_KHR);
                _append(context_attributes, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
            }
            _append(context_attributes, EGL_NONE);
            
#undef _append
        }
        
        
        {
            auto eglDebugMessageControl = (typeof(&eglDebugMessageControlKHR))
                eglGetProcAddress("eglDebugMessageControlKHR");
            assert(eglDebugMessageControl);
            auto r7 = eglDebugMessageControl(egl_debug_proc, 0);
            assert(r7 == EGL_SUCCESS);
        }
        
        {
            int egl_major = -1;
            int egl_minor = -1;
            
            eglInitialize(window.egl_display, &egl_major, &egl_minor);
            assert(egl_major* 1000  + egl_minor >=  1004);
            
            
            printf("EGL_CLIENT_APIS:%s EGL_VENDOR:%s EGL_VERSION:%s EGL_EXTENSIONS:%s \n",
                   eglQueryString(window.egl_display, EGL_CLIENT_APIS),
                   eglQueryString(window.egl_display, EGL_VENDOR),
                   eglQueryString(window.egl_display, EGL_VERSION),
                   eglQueryString(window.egl_display, EGL_EXTENSIONS)
                   );
            
            eglBindAPI(gl_api);
            assert(eglGetError() == EGL_SUCCESS);
        }
        
#if 0
        {
            EGLConfig configs[64];
            int config_count = 0; 
            eglGetConfigs(window.egl_display, configs,
                          sizeof configs/sizeof configs[0],
                          &config_count);
            
            for (int i=0;i<config_count;i++) {
                int v;
#define _P(m_a) \
                eglGetConfigAttrib(window.egl_display, configs[i], m_a, &v); \
                printf( #m_a ": %d\n", v)
                
                _P(EGL_DEPTH_SIZE);
                _P(EGL_SAMPLE_BUFFERS);
                _P(EGL_SAMPLES);
#undef _P
            }
        }
#endif
        
        {
            EGLint num_config;
            
            eglChooseConfig(window.egl_display, config_attributes, 0, 0, &num_config);
            //eglChooseConfig(window.egl_display, 0, 0, 0, &num_config);
            printf("Number of configs %d\n", num_config);
            
            eglChooseConfig(window.egl_display, config_attributes, &window.egl_config, 1, &num_config);
            
            
            
            window.egl_context = eglCreateContext(
            
                window.egl_display, window.egl_config, 
                EGL_NO_CONTEXT, context_attributes);
            auto r5 = eglGetError();
            assert(r5 == EGL_SUCCESS);
            assert(eglGetError() == EGL_SUCCESS);
            assert(window.egl_context != EGL_NO_CONTEXT);
            
            
            
        }
        
    }
    
    
    
    bitmap_initialize(window);
    
    
    return (BarclayWindow*)&window;
}

extern "C" void barclay_window_frame_send(BarclayWindow*window_, 
                                          char wait_for_input) { 
    auto&window = *(WindowPrivate*)window_;
    
    if (window.egl_surface) {
        
        barclay_window_opengl_make_current(&window.p);
        glFlush();
        eglSwapBuffers(window.egl_display, window.egl_surface);
        
#if 1
        for (u32 row=0;row<window.p.height;row++) {
            auto row_pixels = (u8*)window.p.pixels + sizeof(u32) * window.p.width * row;
            glReadPixels(0, window.p.height-(s32)row, window.p.width, 1, GL_RGBA, GL_UNSIGNED_BYTE, row_pixels);
        }
#endif
        
        
    }
    
    {
        
        ClientToServerPacket _p = {
            .wait_for_input = (bool)wait_for_input,
        };
        
        if (!window.waiting_for_flip) {
            // TODO, do we want to ingore this until we recv a frame?
            swap(window.bitmap_front, window.p.pixels);
            window.waiting_for_flip = 1;
        }
        
        
        auto r11 = write(window.p.socket_fd, &_p, sizeof _p);
        assert(r11 == sizeof _p);
        
        
    }
}
extern "C" void barclay_window_frame_recv(BarclayWindow*window_) { 
    auto&window = *(WindowPrivate*)window_;
    
    
    auto&_p_buf = window._p_buf;
    auto&_c_buf = window._c_buf;
    
    
    iovec _iov = {
        .iov_base = window._p_buf,
        .iov_len  = _p_buf_SIZE,
    };
    auto&_chdr = *(cmsghdr*)(void*)window._c_buf = {
        .cmsg_level = SOL_SOCKET,
        .cmsg_type = SCM_RIGHTS,
        .cmsg_len = CMSG_LEN(sizeof(int) * _fd_buf_SIZE),
    };
    msghdr msg = {
        .msg_iov = &_iov,
        .msg_iovlen = 1,
        .msg_control = window._c_buf,
        .msg_controllen = CMSG_SPACE(sizeof(int) * _fd_buf_SIZE),
    };
    
    auto _p_buf_len = recvmsg(window.p.socket_fd, &msg, MSG_CMSG_CLOEXEC);
    assert(_p_buf_len != -1 );
    
    
    auto _fd_buf  = (int*)(void*)(_c_buf + sizeof _chdr);
    assert((void*)_fd_buf == (void*)CMSG_DATA(&_chdr));
    auto _fd_buf_len = (int*)(void*)(_c_buf + _chdr.cmsg_len) - _fd_buf;
    
    auto _p_next = _p_buf;
    auto _fd_next = _fd_buf;
    
    auto& _p_header = *(Packet_Header*)(void*)_p_next;
    _p_next += sizeof _p_header;
    assert(_p_next - _p_buf <= _p_buf_len);
    
    assert(_p_header.version == CLIENT_PROTOCOL_VERSION);
    
    window.p.input_mouse_x = _p_header.mouse_pos.x;
    window.p.input_mouse_y = _p_header.mouse_pos.y;
    
    window.p.has_focus = _p_header.has_focus;
    
    if (_p_header.did_resize) {
        bitmap_cleanup(window);
        
        auto&_p_resize = *(Packet_Size*)(void*)_p_next;
        _p_next += sizeof _p_resize;
        window.bitmap_fd = *_fd_next++;
        assert(_p_next  - _p_buf  <= _p_buf_len); 
        assert(_fd_next - _fd_buf <= _fd_buf_len);
        
        window.p.width  = (u16)_p_resize.size.w;
        window.p.height = (u16)_p_resize.size.h;
        
        bitmap_initialize(window);
        
    }
    
    //printf("PACKET fd:%d raw_len:%d\n", window.socket_fd, _p_header.scancode_len);
    
    window.p.input_scan_codes     = (u16*)(void*)_p_next;
    window.p.input_scan_codes_len = _p_header.scancode_len /2;
    _p_next                     += _p_header.scancode_len;
    
    window.p.input_text     = (char*)_p_next;
    window.p.input_text_len = _p_header.text_len;
    _p_next               += _p_header.text_len;
    
    if (!_p_header.text_len    ) window.p.input_text       = (char*)(void*)null_value;
    if (!_p_header.scancode_len) window.p.input_scan_codes = (u16* )(void*)null_value;
    
    
    window.waiting_for_flip = 0;
}


void barclay_window_release(BarclayWindow*window_) { 
    auto&window = *(WindowPrivate*)window_;
    
    bitmap_cleanup(window);
    close(window.p.socket_fd);
    free(&window);
}



void barclay_window_opengl_make_current(BarclayWindow*window_) { 
    auto&window = *(WindowPrivate*)window_;
    eglMakeCurrent(window.egl_display, window.egl_surface, window.egl_surface, window.egl_context);
}

