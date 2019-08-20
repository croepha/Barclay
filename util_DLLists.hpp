// Copyright 2019 David Butler <croepha@gmail.com>

#pragma once

#include "util_types.hpp"

struct DLListInternal {
    /* e: existing item 
      *  n: new item 
    *  o: offset to prev_next which should be a void*[2] 
    */
    
    void*first,*last; size_t len; 
    bool debug_initialized;
    u8 _pad0[7];
    void insert_after(void*e , void*n, size_t o);
    void insert_before(void*e, void*n, size_t o);
    void remove(void*e, size_t o);
    void append(void*n, size_t o);
    void prepend(void*n, size_t o);
    void _set_first_last(void*n, size_t o);
    void init_from_join(DLListInternal&left, DLListInternal&right, size_t o);
    void initialize();
    void _debug_check();
};

template<class T, T*(T::*PREV_NEXT)[2] = &T::__dll_prev_next> struct DLList {
    union { 
        struct {
            T*first,*last;  size_t len;
            bool debug_initialized;
            u8 _pad0[7];
        };
        DLListInternal _i;
    };
    static constexpr size_t _pno() { auto t = (T*)0x1000; return (size_t)((u8*)(t->*PREV_NEXT) - (u8*)(t)); }
    auto insert_after (T*ex, T*nw) { _i.insert_after (ex, nw, _pno()); return nw;  }
    auto insert_before(T*ex, T*nw) { _i.insert_before(ex, nw, _pno()); return nw;  }
    auto remove       (T*ex)       { _i.remove       (ex,     _pno());             }
    auto append       (T*nw)       { _i.append       (nw,     _pno()); return nw;  }
    auto prepend      (T*nw)       { _i.prepend      (nw,     _pno()); return nw;  }
    
    
    // This moves all elements from left and right to the result
    auto operator+(DLList<T>&rt) { DLList<T> r; r._i.init_from_join(_i, rt._i, _pno()); return r; }
    
    static T* _pn(T*i, int pn) { if(!i) { return 0; } return (i->*PREV_NEXT)[pn]; }
    static T* prev(T*i) { return (i->*PREV_NEXT)[0]; }
    static T* next(T*i) { return (i->*PREV_NEXT)[1]; }
    
    struct IteratorAtBefore {};
    struct IteratorAtEnd    {};
    struct Iterator { T*i; T*p; T*n;
        bool operator!=(Iterator&o) { return i!=o; }
        bool operator==(IteratorAtEnd   &) { return   !i && !n ; }
        bool operator==(IteratorAtBefore&) { return   !i && !p ; }
        bool operator!=(IteratorAtEnd   &) { return !(!i && !n); }
        bool operator!=(IteratorAtBefore&) { return !(!i && !p); }
        void operator++()               { p=i;i=n;n=_pn(i, 1); } 
        void operator--()               { n=i;i=p;p=_pn(i, 0); }
        operator T* () { return i; }
        T&operator*()                   { return *i; }
        
        
    };
    static Iterator iterator(T*i) { return { .i=i, .n=_pn(i, 1), .p=_pn(i, 0) }; }
    Iterator    begin() { _i._debug_check(); return iterator(first); }
    IteratorAtEnd end() { _i._debug_check(); return {}; }
    
    void initialize() { _i.initialize(); }
    void release   () { }
};




