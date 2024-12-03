#include <jni.h>
#include <string>
#include "MediaController.h"
#include "Error.h"
#include "Util.h"

//static val
JavaVM* gJavaVM = nullptr;
jobject nativeMediaCallback = nullptr;
std::shared_ptr<MediaController> mediaController = nullptr;
bool isProcessing = false;
std::mutex mutex;

void callbackToJVM(int ret, const std::string &extra) {
    if(nativeMediaCallback != nullptr && gJavaVM != nullptr) {
        JNIEnv* env;
        jint result = gJavaVM->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) {
            // 处理错误
            return;
        }
        jclass clazz = env->GetObjectClass(nativeMediaCallback);
        jmethodID methodID = env->GetMethodID(clazz, "onEncodeFinish", "(ILjava/lang/String;)V");
        jstring jResultMessage = env->NewStringUTF(extra.c_str());
        env->CallVoidMethod(nativeMediaCallback, methodID, ret, jResultMessage);

        env->DeleteLocalRef(jResultMessage);
        env->DeleteLocalRef(clazz);
        gJavaVM->DetachCurrentThread();
    }
}

class MyIEncodeCallback: public IEncodeCallback {
public:
    void onEncodeStart() override {

    }
    void onEncodeFinish(int ret, const std::string &encodeFile) override {
        callbackToJVM(ret, encodeFile);
        std::lock_guard<std::mutex> lock(mutex);
        isProcessing = false;
    }
};

int initMediaControllerIfNeed() {
    std::lock_guard<std::mutex> lock(mutex);
    if(isProcessing) {
        return PROCESSING;
    }
    if(mediaController == nullptr) {
        mediaController = std::make_shared<MediaController>();
        mediaController->setEncodeCallback(std::make_shared<MyIEncodeCallback>());
    }
    return SUC;
}

int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath) {
    int ret = initMediaControllerIfNeed();
    if(ret == SUC) {
        ret = mediaController->encodeImgToVideo(imgInputPath, videoOutputPath);
        isProcessing = true;
    }
    return ret;
}

int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                             const std::string &videoOutputPath) {
    int ret = initMediaControllerIfNeed();
    if(ret == SUC) {
        ret = mediaController->encodeImgAndAudioToVideo(imgInputPath, audioInputPath, videoOutputPath);
        isProcessing = true;
    }
    return ret;
}

int encodeVideoToVideo(const std::string &videoInputPath, const std::string &videoOutputPath) {
    int ret = initMediaControllerIfNeed();
    if(ret == SUC) {
        ret = mediaController->encodeVideoToVideo(videoInputPath, videoOutputPath);
        isProcessing = true;
    }
    return ret;
}

// JNI_OnLoad 函数
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    // 保存 JavaVM 指针到全局变量
    gJavaVM = vm;

    // 返回 JNI 版本号，这里使用 JNI_VERSION_1_6
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegEncodeImgToVideo(JNIEnv *env, jobject thiz,
                                                                jstring img_path,
                                                                jstring output_path) {
    const char* inputPath = env->GetStringUTFChars(img_path, nullptr);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    int ret = encodeImgToVideo(std::string(inputPath), std::string(outputPath));
    env->ReleaseStringUTFChars(img_path, inputPath);
    env->ReleaseStringUTFChars(output_path, outputPath);
    return ret == 0;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegEncodeImgAndAudioToVideo(JNIEnv *env, jobject thiz,
                                                                        jstring img_input_path,
                                                                        jstring audio_input_path,
                                                                        jstring output_path) {
    const char* imgPath = env->GetStringUTFChars(img_input_path, nullptr);
    const char* audioPath = env->GetStringUTFChars(audio_input_path, nullptr);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    int ret = encodeImgAndAudioToVideo(std::string(imgPath),
                                       std::string(audioPath),
                                       std::string(outputPath));
    env->ReleaseStringUTFChars(img_input_path, imgPath);
    env->ReleaseStringUTFChars(output_path, outputPath);
    env->ReleaseStringUTFChars(audio_input_path, audioPath);
    return ret == 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegSetNativeCallback(JNIEnv *env, jobject thiz,
                                                                 jobject callback) {
    if(nativeMediaCallback != nullptr) {
        env->DeleteGlobalRef(nativeMediaCallback);
        nativeMediaCallback = nullptr;
    }
    if(callback != nullptr) {
        nativeMediaCallback = env->NewGlobalRef(callback);
    }
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegEncodeVideoToVideo(JNIEnv *env, jobject thiz,
                                                                  jstring video_input_path,
                                                                  jstring output_path) {
    const char* inputPath = env->GetStringUTFChars(video_input_path, nullptr);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    int ret = encodeVideoToVideo(std::string(inputPath), std::string(outputPath));
    env->ReleaseStringUTFChars(video_input_path, inputPath);
    env->ReleaseStringUTFChars(output_path, outputPath);
    return ret == 0;
}