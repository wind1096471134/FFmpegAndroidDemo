//
// Created by allan on 2024/12/11.
//

#include <jni.h>
#include <string>
#include "MediaPlayer.h"
#include "Error.h"
#include "Util.h"
#include "android/native_window_jni.h"

JavaVM* gJavaVM = nullptr;
std::shared_ptr<MediaPlayer> mediaPlayer = nullptr;
std::mutex mutexPlayer;
jobject gAudioTrackIns = nullptr;
jobject nativePlayerStateCallback = nullptr;

class MyNativePlayerStateCallback: public IPlayerStateCallback {
private:
    void callbackToJVM(int state) {
        if(nativePlayerStateCallback != nullptr && gJavaVM != nullptr) {
            JNIEnv* env = getEnvThisThread();
            bool envNewCreate = false;
            if(env == nullptr) {
                jint result = gJavaVM->AttachCurrentThread(&env, nullptr);
                if (result != JNI_OK) {
                    // 处理错误
                    return;
                }
                envNewCreate = true;
            }

            jclass clazz = env->GetObjectClass(nativePlayerStateCallback);
            jmethodID methodID = env->GetMethodID(clazz, "onStateChange", "(I)V");
            env->CallVoidMethod(nativePlayerStateCallback, methodID, state);

            env->DeleteLocalRef(clazz);
            if(envNewCreate) {
                gJavaVM->DetachCurrentThread();
            }
        }
    }
public:
    void onStateChange(PlayState state) override {
        callbackToJVM(state);
    }
};
void playVideo(const std::string &videoInputPath, ANativeWindow *nativeWindow, jobject audioTrackIns) {
    std::lock_guard<std::mutex> lockGuard(mutexPlayer);
    if (mediaPlayer == nullptr) {
        mediaPlayer = std::make_shared<MediaPlayer>(nativeWindow, std::make_shared<NativeAudioTrackWrapper>(audioTrackIns, gJavaVM));
    }
    mediaPlayer->setPlayerStateCallback(std::make_shared<MyNativePlayerStateCallback>());
    mediaPlayer->play(const_cast<std::string &>(videoInputPath));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_PlayerActivity_ffmpegPlayVideo(JNIEnv *env, jobject thiz,
                                                           jstring file_url, jobject surface,
                                                           jobject audioTrackIns,
                                                           jobject playerStateCallback) {
    putEnvThisThread(env);
    const char *inputPath = env->GetStringUTFChars(file_url, nullptr);
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (gAudioTrackIns != nullptr) {
        env->DeleteLocalRef(gAudioTrackIns);
        gAudioTrackIns = nullptr;
    }
    gAudioTrackIns = env->NewGlobalRef(audioTrackIns);
    if(nativePlayerStateCallback != nullptr) {
        env->DeleteGlobalRef(nativePlayerStateCallback);
        nativePlayerStateCallback = nullptr;
    }
    if(playerStateCallback != nullptr) {
        nativePlayerStateCallback = env->NewGlobalRef(playerStateCallback);
    }
    playVideo(std::string(inputPath), nativeWindow, gAudioTrackIns);
    ANativeWindow_release(nativeWindow);
    env->ReleaseStringUTFChars(file_url, inputPath);
    putEnvThisThread(nullptr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_PlayerActivity_ffmpegPlayRelease(JNIEnv *env, jobject thiz) {
    putEnvThisThread(env);
    std::lock_guard<std::mutex> lockGuard(mutexPlayer);
    if (mediaPlayer != nullptr) {
        mediaPlayer->release();
        mediaPlayer = nullptr;
    }
    if (gAudioTrackIns != nullptr) {
        env->DeleteGlobalRef(gAudioTrackIns);
        gAudioTrackIns = nullptr;
    }
    if(nativePlayerStateCallback != nullptr) {
        env->DeleteGlobalRef(nativePlayerStateCallback);
        nativePlayerStateCallback = nullptr;
    }
    putEnvThisThread(nullptr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_PlayerActivity_ffmpegPlayPause(JNIEnv *env, jobject thiz) {
    putEnvThisThread(env);
    std::lock_guard<std::mutex> lockGuard(mutexPlayer);
    if (mediaPlayer != nullptr) {
        mediaPlayer->pause();
    }
    putEnvThisThread(nullptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_PlayerActivity_ffmpegPlayResume(JNIEnv *env, jobject thiz) {
    putEnvThisThread(env);
    std::lock_guard<std::mutex> lockGuard(mutexPlayer);
    if (mediaPlayer != nullptr) {
        mediaPlayer->resume();
    }
    putEnvThisThread(nullptr);
}