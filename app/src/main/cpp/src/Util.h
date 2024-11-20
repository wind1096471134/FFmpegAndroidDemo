//
// Created by allan on 2024/11/14.
//

#ifndef FFMPEGDEMO_UTIL_H
#define FFMPEGDEMO_UTIL_H

#include "stdlib.h"

bool strEndWith(const char *originStr, const char * suffix);
void log(const char *tag, const char *msg, int ret1 = 0, int re2 = 0);
void log(const char *tag, const char *fmtMsg, va_list vl);

#endif //FFMPEGDEMO_UTIL_H
