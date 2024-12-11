//
// Created by allan on 2024/11/14.
//

#ifndef FFMPEGDEMO_UTIL_H
#define FFMPEGDEMO_UTIL_H

#include "stdlib.h"
#include "jni.h"
extern "C" {
#include "libavutil/rational.h"
#include "libavutil/mathematics.h"
};

extern JavaVM* gJavaVM;

JNIEnv* getEnvThisThread();
void putEnvThisThread(JNIEnv *env);
int64_t rescaleTimestamp(int64_t ts, AVRational tb_src, AVRational tb_dst);
bool strEndWith(const char *originStr, const char * suffix);
long long getCurTimestamp();
void log(const char *tag, const char *msg, int ret1 = 0, int re2 = 0);
void log(const char *tag, const char *msg, const char *ret1, int ret2 = 0);


#endif //FFMPEGDEMO_UTIL_H
