// Copyright 2019 David Butler <croepha@gmail.com>

#include <assert.h>
#include "util_DLLists.hpp"


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"



#define P(m_i) ((void**)((u8*)m_i + o))[0]
#define N(m_i) ((void**)((u8*)m_i + o))[1]

void DLListInternal::insert_after(void*e, void*n, size_t o) {
    assert(debug_initialized);
    assert(P(n) == 0);
    assert(N(n) == 0);
    assert(e!=n);
    P(n)              = e;
    N(n)              = N(e);
    if (N(e)) P(N(e)) = n;
    else   last       = n;
    N(e)              = n;
    len++;
    _debug_check();
}
void DLListInternal::insert_before(void*e, void*n, size_t o) {
    assert(debug_initialized);
    assert(P(n) == 0);
    assert(N(n) == 0);
    assert(e!=n);
    N(n)              = e;
    P(n)              = P(e);
    if (P(e)) N(P(e)) = n;
    else   first      = n;
    P(e)              = n;
    len++;
    _debug_check();
}
void DLListInternal::remove(void*e, size_t o) {
    assert(debug_initialized);
    assert(len);
    len--;
    if (P(e)) N(P(e)) = N(e);
    else   first      = N(e);
    if (N(e)) P(N(e)) = P(e);
    else   last       = P(e);
    P(e) = 0;
    N(e) = 0;
    _debug_check();
}
void DLListInternal::_set_first_last(void*n, size_t o) {
    assert(debug_initialized);
    assert(!len);
    assert(!first);
    assert(!last);
    assert(P(n) == 0);
    assert(N(n) == 0);
    
    first = last = n;
    N(n) = P(n) = 0;
    len++;
    _debug_check();
}
void DLListInternal::append(void*n, size_t o) {
    assert(debug_initialized);
    if (len) return insert_after(last, n, o);
    else            _set_first_last(n, o);
}
void DLListInternal::prepend(void*n, size_t o) {
    assert(debug_initialized);
    if (len) return insert_before(first, n, o);
    else            _set_first_last(n, o);
}
void DLListInternal::init_from_join(DLListInternal&left, DLListInternal&right, size_t o) {
    debug_initialized = true;
    
    left._debug_check();
    right._debug_check();
    
    
    first = left.first;
    last  = right.last;
    len   = left.len + right.len;
    
    
    if        ( left.len &&  right.len) {
        N(left .last ) = right.first;
        P(right.first) = left .last;
    } else if (!left.len && !right.len) {
        // nothing
    } else if ( left.len && !right.len) {
        last  = left.last;
    } else if (!left.len &&  right.len) {
        first = right.first;
    } else { assert(0);} 
    
    
    left.initialize();  // clear
    right.initialize();  // clear
    
    left._debug_check();
    right._debug_check();
    _debug_check();
    
}
void DLListInternal::initialize() { 
    first=0; last=0; len=0; debug_initialized=true;
}

void DLListInternal::_debug_check() {
    assert(debug_initialized);
    if (first && first == last) { assert(len == 1); }
    if (len) {
        assert(first);
        assert(last);
    }
    
}

