//
// Created by allan on 2024/12/8.
//

#include "NativeAudioTrackWrapper.h"
#include "exception"

NativeAudioTrackWrapper::NativeAudioTrackWrapper(jobject nativeAudioTrackIns, JavaVM *gJavaVM):
    nativeAudioTrackIns(nativeAudioTrackIns), gJavaVM(gJavaVM){
}

NativeAudioTrackWrapper::~NativeAudioTrackWrapper() {
}

void NativeAudioTrackWrapper::playStart(int sampleRate, int channelConfig, int audioFormat) {

    callJavaMethod("playStart", "(III)V", [&](JNIEnv* env, jobject obj, jmethodID methodId) {
        env->CallVoidMethod(obj, methodId, sampleRate, channelConfig, audioFormat);
    });
}

void NativeAudioTrackWrapper::playFrame(uint8_t *data, int size) {
    callJavaMethod("playFrame", "([BI)V", [&](JNIEnv* env, jobject obj, jmethodID methodId) {
        jbyteArray newJData = env->NewByteArray(size);
        env->SetByteArrayRegion(newJData, 0, size, reinterpret_cast<const jbyte *>(data));
        env->CallVoidMethod(obj, methodId, newJData, size);
        env->DeleteLocalRef(newJData);
    });
}

void NativeAudioTrackWrapper::playEnd() {
    callJavaMethod("playEnd", "()V", [&](JNIEnv* env, jobject obj, jmethodID methodId) {
        env->CallVoidMethod(obj, methodId);
    });
}

void NativeAudioTrackWrapper::callJavaMethod(const char *methodName, const char *methodSig,
                                             std::function<void(JNIEnv*, jobject, jmethodID)> methodReadyToCall) {
    JNIEnv* env;
    jint result = gJavaVM->AttachCurrentThread(&env, nullptr);
    if (result != JNI_OK) {
        return;
    }
    jclass clazz = env->GetObjectClass(nativeAudioTrackIns);
    jmethodID methodID = env->GetMethodID(clazz, methodName, methodSig);
    methodReadyToCall(env, nativeAudioTrackIns, methodID);
    env->DeleteLocalRef(clazz);
    gJavaVM->DetachCurrentThread();
}


