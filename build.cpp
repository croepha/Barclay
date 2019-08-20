// Copyright 2019 David Butler <croepha@gmail.com>

// TODO is fast-math redundant to Ofast?
//perfsan = " -Ofast -DNDEBUG --rtlib=compiler-rt  -nostdlib++ -Wno-unused-variable  -Wno-unused-command-line-argument -ffast-math ";
//perfsan = "";

//perfsan = " -Ofast --rtlib=compiler-rt  -nostdlib++  -Wno-unused-command-line-argument -ffast-math  ";




cxx("tool_getshader");
ld_exec("tool_getshader");

cflags = " -I /usr/include/drm "; 
cxx("ds_dri");
cflags = 0;

cxx("test_egl_example");
ldflags = " -lEGL -lSDL2 -lGL";
ld_exec("test_egl_example");

hxx("test_server");
cxx("ds_sdl");
pch_file = 0;


cflags = " -I /usr/include/drm "; 
cxx("barclay_client_lib");
cflags = 0;

cxx("keymap");
cxx("window");
//cxx("util_lazy_sdlgl");
cxx("util_gl");
cflags = " -Wno-cast-align ";
cxx("test_server_bug");
cxx("util_DLLists");
cxx("render_draw_texture");
//cxx("render_ascii_mono");
cxx("render_boxes");

_("build $outd/shader_draw_texture.cpp : custom_basic $outd/tool_getshader.exec $ind/render_draw_texture.cpp");
_("build $outd/shader_draw_texture.o   : cxx    $outd/shader_draw_texture.cpp ");

_("build $outd/shader_boxes.cpp : custom_basic $outd/tool_getshader.exec $ind/render_boxes.cpp");
_("build $outd/shader_boxes.o   : cxx    $outd/shader_boxes.cpp ");


cxx("ds_common"); 

cxx("ds_egl");
cxx("ds_sdl_gl33_noegl");

ldflags = " -l EGL -lGL -lgbm -ldrm " PROFILE_LDFLAGS;
ld_exec("ds_dri", "ds_egl", "util_DLLists", // "render_ascii_mono"
        "shader_draw_texture", "render_draw_texture", 
        "shader_boxes", "render_boxes",
        "keymap", "window",
        "misc", "util_gl", "ds_common");

ldflags = "  -l SDL2 -l EGL -lGL " PROFILE_LDFLAGS;
ld_exec("ds_sdl", "ds_egl", "util_DLLists", // "render_ascii_mono"
        "shader_draw_texture", "render_draw_texture", 
        "shader_boxes", "render_boxes",
        "keymap", "window",
        "misc", "util_gl", "ds_common");

ldflags = "  -l SDL2  -lGL " PROFILE_LDFLAGS;
ld_exec("ds_sdl_gl33_noegl", "util_DLLists", // "render_ascii_mono"
        "shader_draw_texture", "render_draw_texture", 
        "shader_boxes", "render_boxes",
        "keymap", "window",
        "misc", "util_gl", "ds_common");

ld_exec("test_server_bug");
ldflags = 0;

cflags = " -Wno-float-equal -Wno-missing-field-initializers";
cxx("test_client");
cflags = 0;
//cxx("test_client_plot");


cc("example_simple");
cc("example_input");
cc("example_multi_window");
ldflags = " ./build/barclay_client_lib.so ";
ld_exec("example_simple");
ld_exec("example_input");
ld_exec("example_multi_window");

// copy("build/barclay_client_lib.so", "libbarclay.so");
// ldflags = " -L./build -l barclay ";

//ldflags = " -ldrm -lgbm ";
ldflags = "  -lGL -lEGL   -lgbm  -ldrm ";
ld_dynamic("barclay_client_lib");
ldflags = " ./build/barclay_client_lib.so -lGL -lEGL -ldl ";
ld_exec("test_client", "ds_egl", "util_gl");
ldflags = 0;
ld_exec("test_client_plot");

ldflags = " ./build/barclay_client_lib.so ";
//cxx("test_client_opengl_tmp");
//ld_exec("test_client_opengl_tmp");
ldflags = 0;



cxx("test_eglstream");
ldflags = "  -l SDL2 -l EGL -lGL " PROFILE_LDFLAGS;
ld_exec("test_eglstream", "ds_sdl", "util_gl", "ds_egl", 
        "shader_draw_texture", "render_draw_texture"
        );


perfsan = "";

cxx("misc");




