#if 0/*
# Copyright 2019 David Butler <croepha@gmail.com>
#set -x
export PATH=/coding/local_prefix/bin:$PATH

set -eEuo pipefail

export NINJA_STATUS="[%f/%t %e] "

mkdir -p build

OUT_DIR=$(cd build; pwd)/
IN_DIR=$(pwd)/

clang++ build_base.cpp \
-Wno-writable-strings \
-D "OUT_DIR=\"$OUT_DIR\"" \
-D "IN_DIR=\"$IN_DIR\"" -o build/build.exec
build/build.exec

# -d explain
ninja  -f "$OUT_DIR"/build.ninja

exit
*/
#endif 
//  This is some generic build stuff that I use accross multiple projects...


//#define USE_LINKER " -fuse-ld=lld -Wl,--start-group "  // for linux
#define USE_LINKER "  "  // for osx

#if 0
#define PROFILE_CFLAGS_ALWAYS  " -I /coding/tools/easy_profiler-v2.0.1-linux_x64-libc_2.23/include/   "
#define PROFILE_CFLAGS  " -D BUILD_WITH_EASY_PROFILER  "
#define PROFILE_LDFLAGS  " /coding/tools/easy_profiler-v2.0.1-linux_x64-libc_2.23/bin/libeasy_profiler.so "
#else 

#define PROFILE_CFLAGS_ALWAYS  " -I /coding/easy_profiler/easy_profiler_core/include  "
#define PROFILE_CFLAGS  " -D BUILD_WITH_EASY_PROFILER  "
#define PROFILE_LDFLAGS  " /coding/easy_profiler/build/bin/libeasy_profiler.so "

#endif

#if 1
#undef PROFILE_CFLAGS  
#undef PROFILE_LDFLAGS 
#define PROFILE_CFLAGS  
#define PROFILE_LDFLAGS 
#endif




#include <stdio.h>
#include <assert.h>

FILE* outf;


char* build_name;
char* perfsan;
char* scp_dest;
char* cflags;
char* ldflags;
char* output_name;
char* pch_file;
char  pch_file_buffer[1024];

char* perfsan_default = "-fsanitize=address,undefined,integer -O1";
char* perfsan_default_no_udefined = "-fsanitize=address -O1";

#define ld_exec(...) __ld_exec({ __VA_ARGS__ })
template<int SIZE> void __ld_exec(char*(&&names)[SIZE]) {
    
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    _("\nbuild $outd/%s%s.exec: ld_exec", names[0], build_name);
    for (int i=0; i<SIZE; i++) {
        _(" $outd/%s%s.o", names[i], build_name);
    }
    _("\n");
#define _a(m_var) ({ if (m_var) { _("  %s = %s\n", #m_var, m_var); } })
    _a(perfsan);
    _a(ldflags);
#undef _a
    
    
    if (scp_dest) {
        _("\nbuild $outd/%s%s.exec.transferred: custom_no_product $outd/%s%s.exec\n", names[0], build_name, names[0], build_name);
        _("  custom_command = scp $outd/%s%s.exec %s\n", names[0], build_name, scp_dest);
        _("  custom_description = Transferring $outd/%s%s.exec\n", names[0], build_name);
    }
    
#undef _
    
}




#define ld_dynamic(...) __ld_dynamic({ __VA_ARGS__ })
template<int SIZE> void __ld_dynamic(char*(&&names)[SIZE]) {
    
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    _("\nbuild $outd/%s%s.so: ld_dynamic", names[0], build_name);
    for (int i=0; i<SIZE; i++) {
        _(" $outd/%s%s.o", names[i], build_name);
    }
    _("\n");
#define _a(m_var) ({ if (m_var) { _("  %s = %s\n", #m_var, m_var); } })
    _a(perfsan);
    _a(ldflags);
#undef _a
    
    
    if (scp_dest) {
        _("\nbuild $outd/%s%s.exec.transferred: custom_no_product $outd/%s%s.exec\n", names[0], build_name, names[0], build_name);
        _("  custom_command = scp $outd/%s%s.exec %s\n", names[0], build_name, scp_dest);
        _("  custom_description = Transferring $outd/%s%s.exec\n", names[0], build_name);
    }
    
#undef _
    
}



#define cxx(...) __cxx({ __VA_ARGS__ })
template<int SIZE> void __cxx(char*(&&names)[SIZE]) {
    
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    if (!output_name) {
        output_name = names[0];
    }
    
    _("\nbuild $outd/%s%s.o: cxx $ind/%s.cpp", output_name, build_name, names[0]);
    
    output_name = 0;
    
    if (SIZE > 1) {
        _(" |");
        for (int i=1; i<SIZE; i++) {
            _(" %s", names[i]);
        }
    }
    
    if (pch_file) {
        if (SIZE == 1) {
            _(" |");
        }
        _(" %s", pch_file);
    }
    
    _("\n");
#define _a(m_var) ({ if (m_var) { _("  %s = %s\n", #m_var, m_var); } })
    _a(perfsan);
    _a(cflags);
#undef _a
    
    if (pch_file) {
        _("  pch_options = -include-pch %s\n", pch_file);
    }
    
    
#undef _
}

#define cc(...) __cc({ __VA_ARGS__ })
template<int SIZE> void __cc(char*(&&names)[SIZE]) {
    
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    if (!output_name) {
        output_name = names[0];
    }
    
    _("\nbuild $outd/%s%s.o: cc $ind/%s.c", output_name, build_name, names[0]);
    
    output_name = 0;
    
    if (SIZE > 1) {
        _(" |");
        for (int i=1; i<SIZE; i++) {
            _(" %s", names[i]);
        }
    }
    
    if (pch_file) {
        if (SIZE == 1) {
            _(" |");
        }
        _(" %s", pch_file);
    }
    
    _("\n");
#define _a(m_var) ({ if (m_var) { _("  %s = %s\n", #m_var, m_var); } })
    _a(perfsan);
    _a(cflags);
#undef _a
    
    if (pch_file) {
        _("  pch_options = -include-pch %s\n", pch_file);
    }
    
    
#undef _
}


void hxx(char*name) {
    
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    if (!output_name) {
        output_name = name;
    }
    
    _("\nbuild $outd/%s%s.hpp.pch: hxx $ind/%s.hpp\n", output_name, build_name, name);
    
#define _a(m_var) ({ if (m_var) { _("  %s = %s\n", #m_var, m_var); } })
    _a(perfsan);
    _a(cflags);
#undef _a
    
#undef _
    
    auto r2 = snprintf(pch_file_buffer, sizeof pch_file_buffer, 
                       "$outd/%s%s.hpp.pch", output_name, build_name );
    assert(r2 < sizeof pch_file_buffer - 1);
    pch_file = pch_file_buffer;
    
    output_name = 0;
    
    
    
}

#define test(m_ef, m_tn, ...) __test(m_ef, m_tn, { __VA_ARGS__ })
template<int SIZE> void __test(char*exec_file, char*test_name, char*(&&inputs)[SIZE]) {
#define _(fmt, ...)  fprintf(outf, fmt, ##__VA_ARGS__);
    
    _("\nbuild $outd/%s%s.test_report: test_run build/%s%s.exec ", 
      test_name, build_name, exec_file, build_name );
    for (int i=0; i<SIZE; i++) {
        _(" $ind/%s%s", inputs[i], build_name);
    }
    _(" | $outd/test_run.exec\n");
#undef _
    
    
}





int main() {
    
    outf = fopen(OUT_DIR "build.ninja", "w");
    assert(outf);
    
#define _(fmt, ...)  fprintf(outf, fmt "\n", ##__VA_ARGS__);
    
    _("ind         = " IN_DIR);
    _("outd        = " OUT_DIR);
    _("builddir    = $outd");
    //_("time        = /usr/bin/time");
    _("cxx_bin     = clang++ --std=gnu++2a");
    _("cc_bin      = clang   --std=gnu99");
    
    
    _("common_flags=  $");
    _("  -I $outd $");
    _("  " PROFILE_CFLAGS_ALWAYS PROFILE_CFLAGS " $");
    _("  -I $ind/external_deps $");
    //_("  -I /coding/misc                                      $");
    _("  -g3 -D_GLIBCXX_DEBUG -ggdb3                          $");
    _("  -Wno-gnu-empty-struct  $");
    _("  -Werror -pedantic -fspell-checking -Wno-newline-eof  $");
    _("  -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-cast-qual -Wno-missing-prototypes $");
    _("  -Wno-documentation-unknown-command $");
    _("  -Wno-missing-variable-declarations $");
    _("  -Wno-padded -Wno-reserved-id-macro -Wno-missing-braces -Wno-double-promotion $");
    _("  -Weverything -Wshadow-all -Wno-writable-strings -Wno-gnu-folding-constant     $");
    _("  -Wno-c99-extensions -Wno-gnu-statement-expression -Wno-gnu-anonymous-struct $");
    _("  -Wno-nested-anon-types -Wno-language-extension-token  -Wno-missing-noreturn         $");
    _("  -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-case-range  -Wno-zero-length-array $");
    _("  -Wno-covered-switch-default -Wno-disabled-macro-expansion $");
    _("  -fno-rtti -fno-exceptions -fPIC  -pthread  ");
    
    _("base_cxx_flags = -Wno-unused-template -Wno-zero-as-null-pointer-constant ");
    
    _("rule cxx");
    _("  command     = $time $cxx_bin $base_cxx_flags $common_flags $cflags $perfsan $");
    _("    $pch_options -c $in -o $out -MF $out.d -MD ");
    _("  description = CXX $out");
    _("  depfile     = $out.d");
    _("  deps        = gcc");
    
    _("rule cc");
    _("  command     = $time $cc_bin $common_flags $cflags $perfsan $");
    _("    $pch_options -c $in -o $out -MF $out.d -MD");
    _("  description = CC $out");
    _("  depfile     = $out.d");
    _("  deps        = gcc");
    
    
    _("rule hxx");
    _("  command     = $time $cxx_bin $base_cxx_flags $common_flags $cflags $perfsan $");
    _("  -x c++-header $in -o $out -MF $out.d -MD");
    _("  description = CXX_PCH $out");
    _("  depfile     = $out.d");
    _("  deps        = gcc");
    
    
    _("rule ld_exec");
    _("  command     = $time $cxx_bin " USE_LINKER "   $common_flags  $ldflags  $perfsan $");
    _("     $in -o $out -MF $out.d ");
    _("  description = LD_EXEC $out");
    
    
    _("rule ld_dynamic");
    _("  command     = $time $cxx_bin -fuse-ld=lld-7 $common_flags  $ldflags $perfsan  $");
    _("    $in --shared -o $out -MF $out.d ");
    _("  description = LD_DYNAMIC $out");
    
    
    _("rule test_run");
    _("  command     = $time $outd/test_run.exec $out $in ");
    _("  description = TEST $in");
    _("  depfile     = $out.d");
    _("  deps        = gcc");
    
    
    
    _("rule custom_no_product");
    _("  command     = $time $custom_command | touch $out");
    _("  description = $custom_description");
    
    _("rule custom");
    _("  command     = $custom_command");
    _("  description = $custom_description");
    
    
    _("rule custom_basic");
    _("  command     = $time $in $out");
    
    
    build_name = "";
    
    perfsan = perfsan_default;
    
    
#include "build.cpp"
    
    
    
    fclose(outf);
}

