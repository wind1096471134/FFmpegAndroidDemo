//
// Created by allan on 2024/11/16.
//

#include <android/log.h>
#include "MediaController.h"
#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "AudioDecoder.h"
#include "Error.h"
#include "Util.h"
#include "functional"
#include "mutex"
extern "C" {
#include "libavutil/log.h"
}

#define LOG_TAG "MediaController"
#define FFMPEG_LOG false

void MediaController::onDecodeFrameCallback(DecodeFrameData &data) {
    log(LOG_TAG, "decode frame", data.avFrame->format, data.mediaType);
    //VideoEncoder videoEncoder;
    VideoEncodeParam videoEncodeParam = {data.avFrame->width, data.avFrame->height, 25, 3000000};
    int ret = videoEncoder_encodeImgToVideo(data.avFrame, videoOutputPath, videoEncodeParam, 5);
    log(LOG_TAG, "encodeImgToVideo", ret);
    if(data.isFinish) {
        videoOutputPath.clear();
    }
}

int MediaController::encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath) {
    if(videoDecoder == nullptr) {
        videoDecoder = new VideoDecoder();
    }
    this->videoOutputPath = videoOutputPath;
    int ret = videoDecoder->decodeFile(imgInputPath, mediaDecodeFrameCallback);

    return ret;
}

int MediaController::encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                                              const std::string &videoOutputPath) {
    if(audioDecoder == nullptr) {
        audioDecoder = new AudioDecoder();
    }
    this->videoOutputPath = videoOutputPath;
    std::function<void(DecodeFrameData &data)> audioDecodeFrameCallback = [&](DecodeFrameData &data) {
        log(LOG_TAG, "decode frame", data.avFrame->format, data.frameIndex);
    };
    int ret = audioDecoder->decodeFile(audioInputPath, audioDecodeFrameCallback);
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
    if(videoDecoder != nullptr) {
        delete videoDecoder;
    }
    if(audioDecoder != nullptr) {
        delete audioDecoder;
    }
    if(videoEncoder == nullptr) {
        delete videoDecoder;
    }
}

MediaController *mediaController = nullptr;
bool isProcessing = false;
std::mutex mutex;

int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath) {
    std::lock_guard<std::mutex> lock(mutex);
    if(isProcessing) {
        return PROCESSING;
    }
    if(mediaController == nullptr) {
        mediaController = new MediaController();
    }
    mediaController->encodeImgToVideo(imgInputPath, videoOutputPath);
    isProcessing = true;
    return SUC;
}

int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                             const std::string &videoOutputPath) {
    std::lock_guard<std::mutex> lock(mutex);
    if(isProcessing) {
        return PROCESSING;
    }
    if(mediaController == nullptr) {
        mediaController = new MediaController();
    }
    mediaController->encodeImgAndAudioToVideo(imgInputPath, audioInputPath, videoOutputPath);
    isProcessing = true;
    return SUC;
}
