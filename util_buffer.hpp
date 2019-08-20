// Copyright 2019 David Butler <croepha@gmail.com>
#pragma once

#include <unistd.h>
#include <string.h>
#include <errno.h>

inline static Buffer buffer_move(Buffer dst,  Buffer src) {
    assert(dst.len >= src.len);
    memmove(dst.start, src.start, src.len);
    return {dst.start, src.len};
}

#if 0
inline static Buffer buffer_before(size_t offset, Buffer container) {
    assert(offset <= container.len);
    Buffer before;
    before.start = (u8*)container.start;
    before.len   = offset;
    return before;
}
#endif


inline static Buffer buffer_after(size_t offset, Buffer container) {
    assert(offset <= container.len);
    Buffer after;
    after.start = (u8*)container.start + offset;
    after.len   = (size_t)((u8*)container.end() - (u8*)after.start);
    return after;
}

inline static Buffer buffer_after(Buffer before, Buffer container) {
    assert(before.start >= container.start);
    assert(before.end() <= container.end());
    return buffer_after((size_t)((u8*)before.end()-(u8*)container.start), container);
}

inline static Buffer read(int fd, Buffer buffer) {
    errno = 0;
    auto new_bytes_count = read(fd, buffer.start, buffer.len);
    if (new_bytes_count == -1) return {};
    assert((size_t)new_bytes_count <= buffer.len);
    Buffer ret = buffer;
    ret.len = (size_t)new_bytes_count;
    return ret;
}

#ifdef _STDIO_H

inline static Buffer read(FILE*f, Buffer buffer) {
    errno = 0;
    auto new_bytes_count = fread(buffer.start, 1, buffer.len, f);
    if (new_bytes_count == 0) return {};
    assert(new_bytes_count >= 0); 
    assert((size_t)new_bytes_count <= buffer.len);
    Buffer ret = buffer;
    ret.len = (size_t)new_bytes_count;
    return ret;
}
inline void write(FILE*f, Buffer buffer) {
    errno = 0;
    auto r1 = fwrite(buffer.start, buffer.len, 1, f);
    assert(r1 == 1 || buffer.len == 0);
}
#endif


inline static ssize_t buffer_find_char(Buffer buffer,  char chr) { // memchr?
    for (size_t i=0; i<buffer.len; i++) {
        if (((char*)buffer.start)[i] == chr) {
            return (ssize_t)i;
        }
    }
    return -1;
}



