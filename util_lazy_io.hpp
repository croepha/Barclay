// Copyright 2019 David Butler <croepha@gmail.com>


#pragma once
#include <stdio.h>
#include <stdlib.h>

inline static auto load_file_lazy(char* path) {
    
    auto f = fopen(path, "r");
    assert(f);
    
    u64   buffer_size = 1024;
    char* buffer      = (char*)malloc(buffer_size);
    u64   buffer_len  = 0;
    
    assert(buffer);
    
    for (;;) {
        
        auto read_byte_count = fread(
            buffer+buffer_len, 1, buffer_size-buffer_len -1, f);
        
        if (read_byte_count == 0 && feof(f)) break;
        
        
        buffer_len += read_byte_count;
        
        if (buffer_size - buffer_len < 256) {
            buffer_size = buffer_size * 2;
            buffer = (char*)realloc(buffer, buffer_size);
            assert(buffer);
        }
    }
    
    buffer[buffer_len] = 0;  // Null terminate
    
    // This is optional, trim up some unused space, only really
    //   relevent if you are going to keep the returned 
    //   buffer for a while...
    buffer = (char*)realloc(buffer, buffer_len + 1);
    assert(buffer);
    
    return Buffer{ buffer, buffer_len };
    
    
}

