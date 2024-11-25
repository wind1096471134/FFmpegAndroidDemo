#include <jni.h>
#include <string>
#include "MediaController.h"
#include "Util.h"

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegEncodeImgToVideo(JNIEnv *env, jobject thiz,
                                                                jstring img_path,
                                                                jstring output_path) {
    const char* inputPath = env->GetStringUTFChars(img_path, nullptr);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    MediaController videoController;
    int ret = videoController.encodeImgToVideo(inputPath, outputPath);
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
    MediaController videoController;
    int ret = videoController.encodeImgAndAudioToVideo(imgPath, audioPath, outputPath);
    env->ReleaseStringUTFChars(img_input_path, imgPath);
    env->ReleaseStringUTFChars(output_path, outputPath);
    env->ReleaseStringUTFChars(audio_input_path, audioPath);
    return ret == 0;
}