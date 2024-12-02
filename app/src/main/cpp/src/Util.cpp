//
// Created by allan on 2024/11/14.
//

#include "Util.h"
#include "cstring"
#include "android/log.h"

bool strEndWith(const char *originStr, const char * suffix) {
    size_t originL = strlen(originStr);
    size_t suffixL = strlen(suffix);
    return strncmp(originStr + originL - suffixL, suffix, suffixL) == 0;
}

void log(const char *tag, const char *msg, int ret1, int ret2) {
    __android_log_print(ANDROID_LOG_INFO, tag, "%s,%d,%d", msg, ret1, ret2);
}

void log(const char *tag, const char *msg, const char *ret1, int ret2) {
    __android_log_print(ANDROID_LOG_INFO, tag, "%s,%s,%d", msg, ret1, ret2);
}

int64_t rescaleTimestamp(int64_t ts, AVRational tb_src, AVRational tb_dst) {
    int64_t new_ts = av_rescale_q_rnd(ts, tb_dst, tb_src, AV_ROUND_NEAR_INF);

    return new_ts;
}
