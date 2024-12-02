//
// Created by allan on 2024/11/16.
//

#include <android/log.h>
#include "MediaController.h"
#include "VideoDecoder.h"
#include "VideoEncoder.h"
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
    if(data.isFinish) {
        log(LOG_TAG, "decode frame finish", data.mediaType);
        if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
            videoDecodeEnd = true;
        } else if (data.mediaType == AVMEDIA_TYPE_AUDIO) {
            audioDecodeEnd = true;
        }
    } else {
        if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
            for(int simulatePts = 0; simulatePts < repeatVideoFrameNum; simulatePts++) {
                videoEncoder->encodeFrame({data.avFrame, data.mediaType});
            }
            if(videoEncoder->getEncodeVideoDuration() >= encodeDuration) {
                videoDecodeEnd = true;
                videoDecoder->stopDecode();
            }
        } else {
            videoEncoder->encodeFrame({data.avFrame, data.mediaType});
            if(videoEncoder->getEncodeAudioDuration() >= encodeDuration) {
                audioDecodeEnd = true;
                audioDecoder->stopDecode();
            }
        }
    }
    if(videoDecodeEnd && audioDecodeEnd) {
        videoEncoder->encodeEnd();
        repeatVideoFrameNum = 1;
        videoDecodeEnd = false;
        audioDecodeEnd = false;
    }
}

int MediaController::encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath) {
    repeatVideoFrameNum = encodeFps * encodeDuration;//fps * duration
    audioDecodeEnd = true;//here has no audio, so set it.
    videoEncoder->setEncodeCallback(shared_from_this());
    videoEncoder->encodeStart(videoOutputPath, {true, 3000000, 512, 512, DEFAULT_VIDEO_FPS}, {false});
    videoDecoder->decodeFile(imgInputPath, mediaDecodeFrameCallback);
    return SUC;
}

int MediaController::encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                                              const std::string &videoOutputPath) {
    repeatVideoFrameNum = encodeFps * encodeDuration;//fps * duration
    videoEncoder->setEncodeCallback(shared_from_this());
    videoEncoder->encodeStart(videoOutputPath, {true, 3000000, 512, 512, DEFAULT_VIDEO_FPS}, {true, 400000, 44100, AV_CHANNEL_LAYOUT_STEREO});
    audioDecoder->decodeFile(audioInputPath, mediaDecodeFrameCallback);
    videoDecoder->decodeFile(imgInputPath, mediaDecodeFrameCallback);
    return SUC;
}

void av_log_default_callback(void *avcl, int level, const char *fmt,
                             va_list vl) {
    __android_log_print(ANDROID_LOG_INFO, "ffmpeg", fmt, vl);
}

MediaController::MediaController() {
    if(FFMPEG_LOG) {
        av_log_set_callback(av_log_default_callback);
    }
    videoDecoder = std::make_shared<VideoDecoder>();
    audioDecoder = std::make_shared<VideoDecoder>();
    videoEncoder = std::make_shared<VideoEncoder>();

}

MediaController::~MediaController() {
    if(FFMPEG_LOG) {
        av_log_set_callback(nullptr);
    }
}

void MediaController::onEncodeStart() {
    if(outsideEncodeCallback != nullptr) {
        outsideEncodeCallback->onEncodeStart();
    }
}

void MediaController::onEncodeFinish(int ret, const std::string &encodeFile) {
    log(LOG_TAG, "onEncodeFinish", encodeFile.data(), ret);
    if(outsideEncodeCallback != nullptr) {
        outsideEncodeCallback->onEncodeFinish(ret, encodeFile);
    }
}

void MediaController::setEncodeCallback(std::shared_ptr<IEncodeCallback> callback) {
    outsideEncodeCallback = callback;
}
