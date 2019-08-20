// Copyright 2019 David Butler <croepha@gmail.com>

#define _ANON_START namespace {
#define _ANON_END   }

#include <stdio.h>

#include "util_types.hpp"
#include "keymap.hpp"
#include <easy/profiler.h>

// #include <linux/input-event-codes.h>
#define KEY_MAX                 0x2ff


extern u16 frame_time_delta_ms;
extern bool _debug_ignoring_events;

_ANON_START



u16 scancode_down_ms[KEY_MAX + 1];
bool numlock_active;
bool capslock_active;
bool input_should_capture_text = 1;


const int text_input_buffer_SIZE = 8;
char text_input_buffer[text_input_buffer_SIZE + 1];
int text_input_buffer_len;


void add_char(char c) {
    assert(text_input_buffer_len < text_input_buffer_SIZE);
    text_input_buffer[text_input_buffer_len++] = c;
}

void press(u16 scancode) {
    if (scancode < 256 && input_should_capture_text) {
        
        
        switch ((key_map_special)scancode) {
            case key_map_special::CAPSLOCK: {
                capslock_active =! capslock_active;
            } break;
            case key_map_special::NUMLOCK: {
                numlock_active =! numlock_active;
            } break;
            case key_map_special::TAB:
            case key_map_special::SHIFT_L:
            case key_map_special::SHIFT_R:
            default: {
                
                bool is_shifted = 
                    scancode_down_ms[(int)key_map_special::SHIFT_L] != 0xFFFF || 
                    scancode_down_ms[(int)key_map_special::SHIFT_R] != 0xFFFF;
                
                char numpad_sym = 0;
                if (numlock_active) {
                    numpad_sym = key_map_numpad[scancode];
                }
                char unlocked_sym;
                if (is_shifted || capslock_active) {
                    unlocked_sym  = key_map_unlocked_shifted[scancode];
                } else {
                    unlocked_sym  = key_map_unlocked_unshifted[scancode];
                }
                char locked_sym;
                if (is_shifted) {
                    locked_sym  = key_map_shifted[scancode];
                } else {
                    locked_sym  = key_map_unshifted[scancode];
                }
                
                if (numpad_sym) {
                    add_char(numpad_sym);
                } else if (unlocked_sym) {
                    add_char(unlocked_sym);
                } else if (locked_sym) {
                    add_char(locked_sym);
                } else {
                }
            } break;
        }
    }
}

_ANON_END


bool keymap_is_pressed(u16 scancode) {
    return scancode_down_ms[scancode] != 0xFFFF;
    
    
}

bool keymap_is_pressed_this_frame(u16 scancode) {
    
    
#if 0
    if (scancode_down_ms[scancode] != 0xFFFF &&
        scancode_down_ms[scancode] != 0) {
        printf("keymap_is_pressed_this_frame MISS %hu\n", 
               scancode_down_ms[scancode] );
    }
#endif
    
    return scancode_down_ms[scancode] == 0;
}


void keymap_scancode(u16 scancode, s32 raw_value) {
    
    assert(raw_value >= 0);
    assert(raw_value <= 3);
    // printf("Got scancode %hu\n", scancode);
    
    assert(scancode <= KEY_MAX);
    
    bool  down_old  = scancode_down_ms[scancode] != 0xFFFF;
    bool  down_new  = !!raw_value;
    
    if (down_old != down_new) {
        
        //printf("Key value changed: %d %d\n", scancode, new_value);
        
        if (down_new) {
            press(scancode);
        }
        
        if (down_new) {
            scancode_down_ms[scancode] = 0;
        } else {
            scancode_down_ms[scancode]  = 0xFFFF;
        }
        
    }
    
}




Buffer keymap_frame() {
    
    EASY_FUNCTION();
    
    
    if (_debug_ignoring_events) {
        for (u16 i=0; i<ArrayLen(scancode_down_ms); i++) {
            auto&down = scancode_down_ms[i];
            down = 0xFFFF;
        }
    }
    
    for (u16 i=0; i<ArrayLen(scancode_down_ms); i++) {
        
        auto&down = scancode_down_ms[i];
        if (down != 0xFFFF) {
            //printf("Doing repeat for  %hu\n", i);
            
            down += frame_time_delta_ms;
            
            u16 key_repeat_interval = 35;
            u16 key_repeat_delay    = 250;
            
            if (down >  key_repeat_delay + key_repeat_interval) {
                down -= key_repeat_interval;
                press(i);
            }
        }
        
    }
    
    auto ret_len = text_input_buffer_len;
    text_input_buffer_len = 0;
    return {text_input_buffer, (size_t)ret_len};
}


void keymap_init() {
    for (u16 i=0; i<ArrayLen(scancode_down_ms); i++) {
        auto&down = scancode_down_ms[i];
        down = 0xFFFF;
    }
}



