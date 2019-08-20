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


#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>

#include "barclay.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

void mat_rotate(hmm_mat4* mat, float degrees, float x, float y, float z) {
    hmm_mat4 matd = HMM_Rotate(degrees, HMM_Vec3(x,y,z));
    *mat  = HMM_MultiplyMat4(matd, *mat);
};


GLuint cube_vab0;
int program;
hmm_mat4 projection_lookat;
hmm_mat4 model_view;



void cube_init() {
    
    const char* vertex_shader = 
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
    
    const char* fragment_shader = 
        "#version 100\n"
        "varying highp vec4 vertex_color;\n"
        "void main () {\n"
        "    gl_FragColor = vertex_color;\n"
        "}\n";
    
    
    program = glCreateProgram();
    
    {   int _sh = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(_sh, 1, &vertex_shader, 0);
        glCompileShader(_sh);
        glAttachShader(program, _sh);
    }
    {   int _sh = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(_sh, 1, &fragment_shader, 0);
        glCompileShader(_sh);
        glAttachShader(program, _sh);
    }
    
    
    glLinkProgram(program);
    glValidateProgram(program);
    
    glUseProgram(program);
    
    glGenBuffers(1, &cube_vab0);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vab0);
    
    projection_lookat = HMM_LookAt(HMM_Vec3(0,0,-3), HMM_Vec3(0,0,0), HMM_Vec3(0,10,0));
    model_view = HMM_Mat4d(1);
    
    
}
void cube_draw() {
    
    
    unsigned int shader_mesh_color_location    = glGetAttribLocation (program,"mesh_color");
    unsigned int shader_mesh_xyz_location      = glGetAttribLocation (program,"mesh_xyz");
    unsigned int shader_model_view_location    = glGetUniformLocation(program,"model_view");
    unsigned int shader_projection_location    = glGetUniformLocation(program,"projection");
    
    
    struct Vertex {
        float mesh_xyz[3];
        float mesh_color[4];
    };
    
    
    glEnableVertexAttribArray(shader_mesh_color_location);
    glEnableVertexAttribArray(shader_mesh_xyz_location);
    glVertexAttribPointer(shader_mesh_color_location, 4, GL_FLOAT, 1, sizeof(struct Vertex), 
                          (void*)(offsetof(struct Vertex, mesh_color)));
    glVertexAttribPointer(shader_mesh_xyz_location, 3, GL_FLOAT, 1, sizeof(struct Vertex), 
                          (void*)(offsetof(struct Vertex, mesh_xyz)));
    
    struct Vertex points[] = {
        {{ -1, -1,  1}, {.2f,.2f,.4f,1.f} }, // 0
        {{ -1,  1,  1}, {.2f,.4f,.4f,1.f} }, // 1
        {{  1,  1,  1}, {.4f,.4f,.4f,1.f} }, // 2
        {{  1, -1,  1}, {.4f,.2f,.4f,1.f} }, // 3
        {{ -1, -1, -1}, {.2f,.2f,.2f,1.f} }, // 4
        {{ -1,  1, -1}, {.2f,.4f,.2f,1.f} }, // 5
        {{  1,  1, -1}, {.4f,.4f,.2f,1.f} }, // 6
        {{  1, -1, -1}, {.4f,.2f,.2f,1.f} }, // 7
        
    };
    struct Vertex verts[] = {
        points[0], points[4], points[1], points[5], 
        points[1], points[5], points[2], points[6],
        points[2], points[6], points[3], points[7], 
        points[3], points[7], points[0], points[4],
        points[0], points[1], points[3], points[2], 
        points[4], points[5], points[7], points[6], 
    };
    
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    
    hmm_mat4 projection_perspective = HMM_Perspective(90, 1.0f/1.0f, 0.1f, 10000);
    hmm_mat4 projection = HMM_MultiplyMat4(projection_perspective, projection_lookat);
    
    glUniformMatrix4fv(shader_projection_location, 1, 0, (float*)projection.Elements);;
    glUniformMatrix4fv(shader_model_view_location, 1, 0, (float*)model_view.Elements);;
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(verts)/sizeof(verts[0]));
    
}






int main () {
    
    
    struct BarclayWindow* window = barclay_window_init("gles:2.0 msaa:4 depth debug");
    
    barclay_window_opengl_make_current(window);
    printf("GL_STRINGS: Vendor:%s Renderer:%s Version:%s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_DEPTH_TEST);  
    glEnable(GL_MULTISAMPLE);
    
    
    cube_init();
    
    
    for (;;) {
        unsigned int i;
        for (i=0;i<window->input_text_len;i++) {
            
            switch (window->input_text[i]) {
                case 'w': mat_rotate   (&model_view, -1.0f, 1,0,0); break;
                case 's': mat_rotate   (&model_view,  1.0f, 1,0,0); break;
                case 'a': mat_rotate   (&model_view, -1.0f, 0,1,0); break;
                case 'd': mat_rotate   (&model_view,  1.0f, 0,1,0); break;
                default:;
            }
            
        }
        
        barclay_window_opengl_make_current(window);
        glViewport(0,0,window->width,window->height);
        glClearColor(0,1,0,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        cube_draw();
        
        
        
        barclay_window_frame_send(window);
        barclay_window_frame_recv(window);
    }
}

