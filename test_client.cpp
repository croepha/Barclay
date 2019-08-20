// Copyright 2019 David Butler <croepha@gmail.com>


#include <stdio.h>
#include <poll.h>


#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>


#include "junk/renderdoc_1.4/include/renderdoc.h"



#include "barclay.h"
#include "common.hpp"
#define NO_DEFINE_IMPLEMENTATION
#include "util_gl.cpp"
extern bool _gl_shader_had_error;

#undef NO_DEFINE_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wsign-conversion"
#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"
#pragma clang diagnostic pop


BarclayWindow* windows[128];
unsigned int   window_count;

GLuint cube_vab0;
GLuint program;

hmm_mat4 projection_lookat;
hmm_mat4 model_view;

void cube_init() {
    
    char* vertex_shader = 
        "#version 100\n"
        "attribute highp vec3 mesh_xyz;\n"
        "attribute highp vec4 mesh_color;\n"
        "uniform   highp mat4 model_view;\n"
        "uniform   highp mat4 projection;\n"
        "varying   highp vec4 vertex_color;\n"
        "void main () {\n"
        "    vertex_color = mesh_color;\n"
        "    gl_Position = projection * model_view * vec4(mesh_xyz, 1);\n"
        "}\n"
        ;
    
    char* fragment_shader = 
        "#version 100\n"
        "varying highp vec4 vertex_color;\n"
        "void main () {\n"
        "    gl_FragColor = vertex_color;\n"
        "}\n";
    
    
    program = glCreateProgram();
    
    {   auto _sh = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(_sh, 1, &vertex_shader, 0);
        glCompileShader(_sh);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    {   auto _sh = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(_sh, 1, &fragment_shader, 0);
        glCompileShader(_sh);
        _gl_check_shader(_sh, __FILE__);
        glAttachShader(program, _sh);
    }
    
    
    glLinkProgram(program);
    glValidateProgram(program);
    _gl_check_program(program);
    assert(!_gl_shader_had_error);
    
    glUseProgram(program);
    
    
    assert(!_gl_had_error);
    
    glGenBuffers(1, &cube_vab0);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vab0);
    
    projection_lookat = HMM_LookAt(HMM_Vec3(0,0,-3), HMM_Vec3(0,0,0), HMM_Vec3(0,10,0));
    model_view = HMM_Mat4d(1);
    
    
}
void cube_draw() {
    
    
    auto shader_mesh_color_location    = (u32)glGetAttribLocation (program,"mesh_color");
    auto shader_mesh_xyz_location      = (u32)glGetAttribLocation (program,"mesh_xyz");
    auto shader_model_view_location    = glGetUniformLocation(program,"model_view");
    auto shader_projection_location    = glGetUniformLocation(program,"projection");
    
    
    struct Vertex {
        float mesh_xyz[3];
        float mesh_color[4];
    };
    
    
    glEnableVertexAttribArray(shader_mesh_color_location);
    glEnableVertexAttribArray(shader_mesh_xyz_location);
    glVertexAttribPointer(shader_mesh_color_location, 4, GL_FLOAT, 1, sizeof(Vertex), 
                          (void*)(offsetof(Vertex, mesh_color)));
    glVertexAttribPointer(shader_mesh_xyz_location, 3, GL_FLOAT, 1, sizeof(Vertex), 
                          (void*)(offsetof(Vertex, mesh_xyz)));
    
    Vertex points[] = {
        {{ -1, -1,  1}, {.2f,.2f,.4f,1.f} }, // 0
        {{ -1,  1,  1}, {.2f,.4f,.4f,1.f} }, // 1
        {{  1,  1,  1}, {.4f,.4f,.4f,1.f} }, // 2
        {{  1, -1,  1}, {.4f,.2f,.4f,1.f} }, // 3
        {{ -1, -1, -1}, {.2f,.2f,.2f,1.f} }, // 4
        {{ -1,  1, -1}, {.2f,.4f,.2f,1.f} }, // 5
        {{  1,  1, -1}, {.4f,.4f,.2f,1.f} }, // 6
        {{  1, -1, -1}, {.4f,.2f,.2f,1.f} }, // 7
        
    };
    Vertex verts[] = {
        points[0], points[4], points[1], points[5], 
        points[1], points[5], points[2], points[6],
        points[2], points[6], points[3], points[7], 
        points[3], points[7], points[0], points[4],
        points[0], points[1], points[3], points[2], 
        points[4], points[5], points[7], points[6], 
    };
    
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    
    auto projection_perspective = HMM_Perspective(90, 1.0f/1.0f, 0.1f, 10000);
    auto projection = HMM_MultiplyMat4(projection_perspective, projection_lookat);
    
    
    
    glUniformMatrix4fv(shader_projection_location, 1, 0, (float*)projection.Elements);;
    glUniformMatrix4fv(shader_model_view_location, 1, 0, (float*)model_view.Elements);;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(verts)/sizeof(verts[0]));
    
    
    assert(!_gl_had_error);
    
    
}




#if 0
int epoll_fd = -1;
#endif

BarclayWindow* gl_window;

unsigned char blue = 0;


bool did_first_frame = 0;


void update_and_render_window(BarclayWindow&window, unsigned int window_i) {
    
    bool close_this_window = 0;
    bool spawn_new_window = 0;
    
    if (window.input_text_len || window.input_scan_codes_len) {
        printf("Window: %u, got text: %d `%.*s'  scanncodes: %d: ",
               window_i,
               window.input_text_len,
               window.input_text_len, window.input_text,
               window.input_scan_codes_len);
        for(unsigned int i=0;i<window.input_text_len;i++) {
            if (window.input_text[i] == 'Q') {
                close_this_window = 1;
            }
            if (window.input_text[i] == 'W')
                spawn_new_window = 1;
            
        }
        for (unsigned int i=0;i<window.input_scan_codes_len; i++){
            printf(" %04x ",window.input_scan_codes[i]);
        }
        printf("\n");
        
        
        
    }
    
    unsigned char green = 0;
    if (window.has_focus) green = 0x7f;
    unsigned char red = 0x7f;
    unsigned char alpha = 0xff;
    
    unsigned int back_color = Barclay_PixelRGBA(red, green, blue, alpha);
    
    //printf("red: %d %x %x \n", red, back_color, (((u32)red)<<24));
    
    
    for (int px = 0; px<window.width; px++) {
        for (int py = 0; py<window.height; py++) {
            Barclay_PixelWindowXY(&window, px, py) = back_color;
        }
    }
    
    for (int px = 0; px<10; px++) {
        for (int py = 0; py<10; py++) {
            
            auto sx = px + window.input_mouse_x;
            auto sy = py + window.input_mouse_y;
            
            if (sx <  0) continue;
            if (sy <  0) continue;
            if (sx >= window.width ) continue;
            if (sy >= window.height) continue;
            
            Barclay_PixelWindowXY(&window, sx, sy) = 0xffffffff;
        }
    }
    
    
    if (close_this_window) {
        barclay_window_release(&window);
        
        if (&window == gl_window) {
            gl_window = 0;
        }
        
#if 0
        int window_i = 0;
        for(;;window_i++) {
            assert(window_i);
            if (windows[window_i].fd == window.fd) break;
        }
#endif
        
        windows[window_i] = windows[--window_count];
    }
    if (spawn_new_window) {
        did_first_frame = 0;
        windows[window_count++] = barclay_window_init(0);
    }
    
}


RENDERDOC_API_1_1_2 *rdoc_api = NULL;

#include <string.h>
#include <dlfcn.h>





int main () {
    
    
    {
        auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(RTLD_DEFAULT, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI) {
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
            assert(ret == 1);
            assert(rdoc_api);
        }
    }
    
    
    //epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    
    //make_new_window();
    
    
    
    //windows[window_count++] = barclay_window_init("gles:2.0 msaa:4 depth debug");
    gl_window = windows[window_count++] = barclay_window_init("gles:2.0 depth debug");
    
    
    
    barclay_window_opengl_make_current(windows[0]);
    printf("GL_STRINGS: Vendor:%s Renderer:%s Version:%s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    _gl_init();
    
    glEnable(GL_DEPTH_TEST);  
    glEnable(GL_MULTISAMPLE);
    
    
#if 1
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif
    
    cube_init();
    
    while(window_count) {
        
        
        bool did_atleast_one_window = 0;
        
        for (unsigned int window_i=0;window_i<window_count;window_i++) {
            auto&window = *windows[window_i];
            
            
            pollfd _pfd {
                .fd = window.socket_fd,
                .events = POLLIN,
            };
            
            auto ready = poll(&_pfd, 1, 0);
            if (ready || !did_first_frame) {
                
                if  (ready) {
                    barclay_window_frame_recv(&window);
                }
                
                did_atleast_one_window = 1;
                
                update_and_render_window(window, window_i);
                
                if (&window == gl_window) {
                    
                    auto mat_rotate = [](hmm_mat4& mat, float degrees, float x, float y, float z) {
                        auto matd = HMM_Rotate(degrees, HMM_Vec3(x,y,z));
                        mat  = HMM_MultiplyMat4(matd, mat);
                    };
                    
                    for (u32 i=0;i<window.input_text_len;i++) {
                        
                        switch (window.input_text[i]) {
                            case 'w': mat_rotate   (model_view, -1.0f, 1,0,0); break;
                            case 's': mat_rotate   (model_view,  1.0f, 1,0,0); break;
                            case 'a': mat_rotate   (model_view, -1.0f, 0,1,0); break;
                            case 'd': mat_rotate   (model_view,  1.0f, 0,1,0); break;
                            case 'C': { 
                                if (rdoc_api) {
                                    rdoc_api->TriggerCapture();
                                } else {
                                    printf("rdoc_api is NULL\n");
                                }
                            }  break;
                            
                            default:;
                        }
                    }
                    
                    
                    barclay_window_opengl_make_current(&window);
                    glViewport(0,0,window.width,window.height);
                    glClearColor(0,1,0,1);
                    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                    cube_draw();
                    
                }
                
                barclay_window_frame_send(&window, 1);
            }
        }
        
        did_first_frame = 1;
        if (did_atleast_one_window) {
            blue++;
        }
        
    }
    
    
    
}
