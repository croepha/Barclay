// Copyright 2019 David Butler <croepha@gmail.com>
#pragma once

// TODO: Rename to util_base.hpp? 

typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;
typedef          char          s8;
typedef          short         s16;
typedef          int           s32;
typedef          long long int s64;
typedef     __SIZE_TYPE__      size_t;


#define ArrayLen(m_a) sizeof(m_a)/sizeof(m_a[0])



template<class T> struct V2 {
    union { T x, w; };
    union { T y, h; };
    V2 operator+(V2 o) { return {x+o.x, y+o.y}; }
    V2 operator+(T  o) { return {x+o  , y+o  }; }
    V2 operator-(V2 o) { return {(T)(x-o.x), (T)(y-o.y)}; }
    V2 operator-(T  o) { return {x-o  , y-o  }; }
    V2 operator*(V2 o) { return {x*o.x, y*o.y}; }
    V2 operator*(T  o) { return {x*o  , y*o  }; }
    V2 operator/(V2 o) { return {x/o.x, y/o.y}; }
    V2 operator/(T  o) { return {x/o  , y/o  }; }
};



struct Position   { s32 x, y; };
struct Size       { s32 w, h; };
struct Bounds     { s32 x, y, w, h; };

#if 0
// TODO:
struct Position   { s16 x, y; };
struct Size       { u16 w, h; };
struct Bounds     { s16 x, y, w, h; };
#endif
enum struct Color3:u8;

struct FD { int _;}; 


struct Buffer { void* start; size_t len; 
    template<typename T> void set_space(T&t) { start=&t; len=sizeof(t);}
    u8*bytes() { return (u8*)start; }
    void*end() { return (u8*)start+len;}
    //template<typename T> T* as() { return (T*)start; }
};
inline static Buffer operator"" _B (const char* _, size_t len ) { return {(void*)_, len}; }


struct String { 
    char*start; size_t len;
    operator const char*() { return start; }
    operator char*() { return start; }
    explicit operator bool() { return len; }
    char& operator[] (const size_t i) { return start[i]; }
};



inline static void debug_break() { asm( "int3" ); }
inline static bool is_inside(Position pos, Bounds bounds) { 
    if (pos.x < bounds.x) return 0;
    if (pos.y < bounds.y) return 0;
    if (pos.x > bounds.x + bounds.w) return 0;
    if (pos.y > bounds.y + bounds.h) return 0;
    return 1;
    
}

template<class T, size_t SIZE> inline static constexpr 
size_t acount(T(&)[SIZE]) { return SIZE; }

template<class T1, class T2, class T3> inline void set_minmax(T1&a, T2 low, T3 high) {
    assert(low <=  high);
    if     (a<low ) a = low; 
    else if(a>high) a = high;
}
template<class A, class B> inline void set_min(A&a, B b) { if(a>b) a = b; }
template<class A, class B> inline void set_max(A&a, B b) { if(a<b) a = b; }
template<class A, class ...B> inline void set_min(A&a, B&&...b) {(set_min(a, b), ...);}
template<class A, class ...B> inline void set_max(A&a, B&&...b) {(set_max(a, b), ...);}
template<class A, class ...B> inline A min_of(A a, B&&...b) {set_min(a, b...);return a;}
template<class A, class ...B> inline A max_of(A a, B&&...b) {set_max(a, b...);return a;}


template<class T> inline void swap(T&a, T&b) { 
    auto _swap = a;
    a = b;
    b = _swap;
}


#include <assert.h>
// TODO: Define assert ?

