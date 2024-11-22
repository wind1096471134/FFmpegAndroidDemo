//
// Created by allan on 2024/11/16.
//

#include <android/log.h>
#include "VideoController.h"
#include "Util.h"
#include "functional"
extern "C" {
#include "libavutil/log.h"
}

#define LOG_TAG "VideoController"
#define FFMPEG_LOG true

int VideoController::encodeVideoWithImg(const char *imgInputPath, const char *videoOutputPath) {
    VideoDecoder videoDecoder;
    int decodeFrameNum = 0;
    int ret = 0;
    std::function<void(DecodeFrameData &data)> decodeFrameCallback = [&](DecodeFrameData &data) {
        decodeFrameNum++;
        log(LOG_TAG, "decode frame", data.frameIndex, data.mediaType);
        if(decodeFrameNum == 1) {
            VideoEncoder videoEncoder;
            ret = videoEncoder.encodeImgToVideo(data.avFrame, videoOutputPath);
            log(LOG_TAG, "encodeImgToVideo", ret);
            videoEncoder.destroy();
        }
    };
    ret = videoDecoder.decodeFile(imgInputPath, decodeFrameCallback);
    videoDecoder.destroy();
    return ret;
}

void av_log_default_callback(void *avcl, int level, const char *fmt,
                             va_list vl) {
    __android_log_print(ANDROID_LOG_INFO, "ffmpeg", fmt, vl);
}

VideoController::VideoController() {
    if(FFMPEG_LOG) {
        av_log_set_callback(av_log_default_callback);
    }
}


