// Copyright 2019 David Butler <croepha@gmail.com>


#include <stdio.h>
#include <string.h>
#include "util_types.hpp"


s64 last_line_start;

void search_file_for_string(FILE*file, char* string) {
    char* s=string;
    for(;;) {
        if(!*s) break;
        auto _rc = fgetc(file);
        assert(_rc != EOF);
        if (_rc == '\n') { last_line_start = ftell(file);} 
        if(*s++!=_rc) { s = string; }
    }
}



int main (int, char**args) {
    args++;
    
    auto input_path = *args++;
    assert(input_path);
    auto output_path = *args++;
    assert(output_path);
    assert(!*args);
    
    auto input_file = fopen(input_path, "r");
    assert(input_file);
    auto output_file = fopen(output_path, "w");
    assert(output_file);
    
    
    auto _ext = strrchr(input_path, '.');
    assert(_ext);
    
    auto _fn = strrchr(_ext, '/');
    if(!_fn) _fn = input_path;
    
    auto _rn = strstr(_fn, "render_");
    assert(_rn);
    
    char* shader_base_name = _rn + strlen("render_");
    *_ext = 0;
    
    printf("shader_base_name: %s\n", shader_base_name); 
    
    
    
    
    
    
    
#define _(m_s) search_file_for_string(input_file, m_s)
    
    _("SHADER_VERTEX_BEGIN");
    _("\n");
    
    auto shader_vertex_begin = ftell(input_file);
    
    _("SHADER_FRAGMENT_BEGIN");
    auto shader_vertex_end  = last_line_start;
    _("\n");
    
    auto shader_fragment_begin = ftell(input_file);
    _("SHADER_END");
    auto shader_fragment_end  = last_line_start;
    
    {
        fseek(input_file, shader_vertex_begin, SEEK_SET);
        fprintf(output_file, "const char shader_%s_vertex_[] = {\n", shader_base_name);
        int cols = 0;
        for (int i = 0; i< shader_vertex_end-shader_vertex_begin; i++)  {
            auto _c  = fgetc(input_file);
            assert(_c != EOF);
            cols += fprintf(output_file, "'\\x%02x', ", _c);
            if (cols > 70) {
                fprintf(output_file, "\n");
                cols = 0;
            }
        }
        if (cols > 0) {
            fprintf(output_file, "\n");
            cols = 0;
        }
        fprintf(output_file, " '\\x00' };\n");
        fprintf(output_file, "const char*shader_%s_vertex= shader_%s_vertex_;\n", shader_base_name, shader_base_name);
        
    }
    
    
    { 
        fseek(input_file, shader_fragment_begin, SEEK_SET);
        fprintf(output_file, "char shader_%s_fragment_[] = {\n", shader_base_name);
        int cols = 0;
        for (int i = 0; i< shader_fragment_end-shader_fragment_begin; i++)  {
            auto _c  = fgetc(input_file);
            assert(_c != EOF);
            cols += fprintf(output_file, "'\\x%02x', ", _c);
            if (cols > 70) {
                fprintf(output_file, "\n");
                cols = 0;
            }
        }
        if (cols > 0) {
            fprintf(output_file, "\n");
            cols = 0;
        }
        fprintf(output_file, " '\\x00' };\n");
        fprintf(output_file, "const char*shader_%s_fragment= shader_%s_fragment_;\n", shader_base_name, shader_base_name);
    }
    
    
    auto r1 = fclose(input_file);
    assert(!r1);
    auto r2 = fclose(output_file);
    assert(!r2);
    
    
    
    
}

