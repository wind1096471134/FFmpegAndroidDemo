//
// Created by allan on 2024/12/8.
//

#ifndef FFMPEGDEMO_NATIVEAUDIOTRACKWRAPPER_H
#define FFMPEGDEMO_NATIVEAUDIOTRACKWRAPPER_H

#include "jni.h"
#include "functional"

class NativeAudioTrackWrapper {
private:
    jobject nativeAudioTrackIns = nullptr;
    JavaVM* gJavaVM;
    void callJavaMethod(const char* methodName, const char* methodSig, std::function<void(JNIEnv*, jobject, jmethodID)> methodReadyToCall);
public:
    NativeAudioTrackWrapper(jobject nativeAudioTrackIns, JavaVM* gJavaVM);
    ~NativeAudioTrackWrapper();
    /**
     * @param sampleRate
     * @param channelConfig 1:MONO 2:STEREO
     * @param audioFormat 1:8BIT 2:16BIT
     */
    void playStart(int sampleRate, int channelConfig, int audioFormat);
    void playFrame(uint8_t *data, int size);
    void playEnd();
};
#endif //FFMPEGDEMO_NATIVEAUDIOTRACKWRAPPER_H
