// Copyright 2019 David Butler <croepha@gmail.com>

#pragma once

#include "util_types.hpp"
#include "util_DLLists.hpp"


enum struct PollType {
    //_INVALID,
    ListeningSocket,
    ClientData,
    UEventSocket,
    InputDevice,
};

struct Client {
    PollType  _poll_type;
    s32       fd;
    s32       bitmap_fd;
    V2<s16>   pos;
    V2<s16>   mouse_pos;
    V2<s16>   size;
    V2<s16>   bitmap_size;
    bool      has_focus;
    bool      is_flipping;
    void*     bitmap_front;
    void*     bitmap_back;
    bool      resized;
    u32       gl_texture;
    Client* __dll_prev_next[2];
    Buffer    new_text_input;
    char*     buffered_text_input_start;
    u32       buffered_text_input_size;
    u32       buffered_text_input_len;
    Buffer    new_scancode_input;
    char*     buffered_scancode_input_start;
    u32       buffered_scancode_input_size;
    u32       buffered_scancode_input_len;
    bool      wait_for_input;
};

u32 get_ticks_ms();


extern DLList<Client> clients;


static const u16 SCREEN_WIDTH  = 1280;
static const u16 SCREEN_HEIGHT =  768;


#define _STRINGIFY2(x) #x
#define _STRINGIFY(x) _STRINGIFY2(x)
#define __LINE_AS_STRING__ _STRINGIFY(__LINE__)

#define warn(...) fprintf(stderr, __FILE__ ":" __LINE_AS_STRING__ ":"  __VA_ARGS__)
#define fatal(...) warn(__VA_ARGS__); fprintf(stderr, "\n"); abort();
