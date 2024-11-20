#include <jni.h>
#include <string>
#include "VideoController.h"
#include "Util.h"

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_ffmpegdemo_MainActivity_ffmpegEncoderTest(JNIEnv *env, jobject thiz,
                                                           jstring input_path,
                                                           jstring output_path) {
    const char* inputPath = env->GetStringUTFChars(input_path, nullptr);
    const char* outputPath = env->GetStringUTFChars(output_path, nullptr);
    VideoController videoController;
    int ret = videoController.encodeVideoWithImg(inputPath, outputPath);
    env->ReleaseStringUTFChars(input_path, inputPath);
    env->ReleaseStringUTFChars(output_path, outputPath);
    return ret == 0;
}