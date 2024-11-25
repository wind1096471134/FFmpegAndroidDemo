//
// Created by allan on 2024/11/16.
//

#include <android/log.h>
#include "MediaController.h"
#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "AudioDecoder.h"
#include "Util.h"
#include "functional"
extern "C" {
#include "libavutil/log.h"
}

#define LOG_TAG "MediaController"
#define FFMPEG_LOG false

int MediaController::encodeImgToVideo(const char *imgInputPath, const char *videoOutputPath) {
    VideoDecoder videoDecoder;
    int ret = 0;
    std::function<void(DecodeFrameData &data)> videoDecodeFrameCallback = [&](DecodeFrameData &data) {
        log(LOG_TAG, "decode frame", data.avFrame->format, data.mediaType);
        VideoEncoder videoEncoder;
        VideoEncodeParam videoEncodeParam = {data.avFrame->width, data.avFrame->height, 25, 3000000};
        ret = videoEncoder_encodeImgToVideo(data.avFrame, videoOutputPath, videoEncodeParam, 5);
        log(LOG_TAG, "encodeImgToVideo", ret);
    };
    ret = videoDecoder.decodeFile(imgInputPath, videoDecodeFrameCallback);

    return ret;
}

int MediaController::encodeImgAndAudioToVideo(const char *imgInputPath, const char *audioInputPath, const char *videoOutputPath) {
    AudioDecoder audioDecoder;
    std::function<void(DecodeFrameData &data)> audioDecodeFrameCallback = [&](DecodeFrameData &data) {
        log(LOG_TAG, "decode frame", data.avFrame->format, data.frameIndex);
    };
    int ret = audioDecoder.decodeFile(audioInputPath, audioDecodeFrameCallback);
    return ret;
}

void av_log_default_callback(void *avcl, int level, const char *fmt,
                             va_list vl) {
    __android_log_print(ANDROID_LOG_INFO, "ffmpeg", fmt, vl);
}

MediaController::MediaController() {
    if(FFMPEG_LOG) {
        av_log_set_callback(av_log_default_callback);
    }
}

MediaController::~MediaController() {
    if(FFMPEG_LOG) {
        av_log_set_callback(nullptr);
    }
}


