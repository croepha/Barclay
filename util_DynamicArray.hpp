// Copyright 2019 David Butler <croepha@gmail.com>

#include <stdlib.h>
#include "util_types.hpp"


template<class T> void inline system_alloc(T*&ptr, size_t count) {
    if(ptr || count) {
        ptr = (T*)realloc(ptr, count * sizeof(T)); 
    }
    assert(!ptr != !!count);
}
template<class T> inline T& system_alloc(T t) {
    auto ptr = (T*)malloc(sizeof(T));
    *ptr = t;
    return *ptr;
}


template<typename T> struct DynamicArray { 
    size_t len, cap;
    T     *base;
    void  cap_set(size_t new_cap)  { 
        system_alloc(base, new_cap);  
        cap = new_cap; 
    }
    void len_set(size_t new_len) {
        if (new_len > cap) cap_set(new_len); 
        len = new_len;
        
    }
    auto append() { 
        len++; 
        if (len > cap) cap_set(max_of(len, cap *2)); 
        return &base[len-1]; 
    }
    void append(T*v, size_t count) { 
        len += count;
        if (len > cap) cap_set(max_of(len, cap *2));
        memcpy(base+len-count, v, count*sizeof(T));
    }
    auto append    (T v     ) { auto _ = append(); *_=v; return _; }
    auto& operator[](size_t i) { assert(i<len); return base[i];    }
    auto begin      (        ) { return base;           }
    auto end        (        ) { return base + len;     }
    void initialize() { len=0; cap=0; base=0; }
    void release   () { cap_set(0);          }
};

#if 0


template<auto ::member, typename T> int sort_key(const T*a, const T*b) {
    return (a->*member) < (b->*member);
}
template<auto ::member, typename T> void __cro_qsort_wkey__(T* array, size_t count) {
    cro_qsort(array, count, sort_key<member>);
}
#define cro_qsortk(m_a, m_c, m_key) \
__cro_qsort_wkey__<&std::remove_reference<decltype(*m_a)>::type::m_key>(m_a, m_c);


/*
template<auto T::member, typename T> void cro_qsort(T* array, size_t count) {
    qsort(array, count, sizeof(T), (int (*)(const void *, const void *))sort_key<T, member>);
}
*/

#endif


template<class T, class C> void sort(T* array, size_t count, C c) {
    typedef int (*compar1_t)(T          *, T          *);
    typedef int (*compar2_t)(const void *, const void *);
    qsort(array, count, sizeof(T), (compar2_t)(compar1_t)c);
}
template<class T, class C> void sort(DynamicArray<T>array, C c) {
    sort(array.base, array.len, c);
}



