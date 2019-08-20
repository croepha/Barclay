// Copyright 2019 David Butler <croepha@gmail.com>
#pragma once

#include "util_types.hpp"



template<u64 LEN> struct StackBuffer {
    u8 space[LEN];
    operator Buffer() { return { space, LEN }; }
    Buffer   buffer() { return { space, LEN }; }
};



struct LineReadBuffer {
    Buffer buffer;
    size_t used;
    Buffer line;
    void initialize(Buffer buffer_) { 
        buffer=buffer_; 
        used=0; 
        line={ buffer.start, 0}; 
    } 
};

struct Stream {
    int fd;
    bool read_line(LineReadBuffer&lrbuffer);
    void write(Buffer buffer);
    void close();
};


struct TCPServer {
    int fd;
    static TCPServer listen(u16 port);
    u16 get_port();
    Stream accept();
    void close();
};



struct HashMeow {
    alignas(16) u8 _internal_meow_hash_state[144];
    u8 _internal_end[0];
    
    void initialize();
    // DO NOT MIX THESE TWO UPDATE CALLS!
    void update(Buffer buffer); 
    // void update(u8 buffer[static 64]); // NOT TESTED
    u64  finalize();
};

struct hash_MD5 {
    
    unsigned state[4];
    u64 count;
    union {
        char c[64];
        unsigned i[16];
    } tmp_buffer;
    
    void initialize();
    void update(Buffer buffer);
    void finalize(u8*hash_out16);
};




