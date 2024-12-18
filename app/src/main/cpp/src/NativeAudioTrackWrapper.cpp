//
// Created by allan on 2024/12/8.
//

#include "NativeAudioTrackWrapper.h"
#include "exception"
#include "Util.h"

NativeAudioTrackWrapper::NativeAudioTrackWrapper(jobject nativeAudioTrackIns, JavaVM *gJavaVM):
    gJavaVM(gJavaVM){
    JNIEnv* env = getEnvThisThread();
    if(env != nullptr) {
        this->nativeAudioTrackIns = env->NewGlobalRef(nativeAudioTrackIns);
    }
}

NativeAudioTrackWrapper::~NativeAudioTrackWrapper() {
    JNIEnv* env = getEnvThisThread();
    if(env != nullptr) {
        env->DeleteGlobalRef(nativeAudioTrackIns);
    }
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
    JNIEnv* env = getEnvThisThread();
    bool envNewCreate = false;
    if(env == nullptr) {
        jint result = gJavaVM->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) {
            return;
        }
        envNewCreate = true;
    }
    jclass clazz = env->GetObjectClass(nativeAudioTrackIns);
    jmethodID methodID = env->GetMethodID(clazz, methodName, methodSig);
    methodReadyToCall(env, nativeAudioTrackIns, methodID);
    env->DeleteLocalRef(clazz);
    if(envNewCreate) {
        gJavaVM->DetachCurrentThread();
    }
}


