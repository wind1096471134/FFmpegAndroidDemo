//
// Created by allan on 2024/11/14.
//

#include "Util.h"
#include "cstring"
#include "android/log.h"
#include "chrono"
#include "map"
#include "thread"

std::map<std::thread::id, JNIEnv*> envMap;
std::mutex mapMutex;

// JNI_OnLoad 函数
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    // 保存 JavaVM 指针到全局变量
    gJavaVM = vm;

    // 返回 JNI 版本号，这里使用 JNI_VERSION_1_6
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    gJavaVM = nullptr;
}

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
    int64_t new_ts = av_rescale_q_rnd(ts, av_inv_q(tb_dst), av_inv_q(tb_src), AV_ROUND_NEAR_INF);

    return new_ts;
}

long long getCurTimestamp() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

JNIEnv *getEnvThisThread() {
    auto threadId = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(mapMutex);
    auto ret = envMap.find(threadId);
    if(ret == envMap.end()) {
        return nullptr;
    } else {
        return ret->second;
    }
}

void putEnvThisThread(JNIEnv *env) {
    auto threadId = std::this_thread::get_id();
    std::lock_guard<std::mutex> lock(mapMutex);
    if(env == nullptr) {
        envMap.erase(threadId);
    } else {
        envMap[threadId] = env;
    }
}

