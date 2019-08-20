// Copyright 2019 David Butler <croepha@gmail.com>



__asm__(".symver powf,powf@GLIBC_2.2.5");


#include <math.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-macros"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wdouble-promotion"




#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_truetype.h"


