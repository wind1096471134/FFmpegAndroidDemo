//
// Created by allan on 2024/12/4.
//

#include "MediaPlayer.h"
#include "Util.h"
extern "C" {
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#define LOG_TAG "MediaPlayer"

void MediaPlayer::play(std::string& playUrl) {
    if(videoDecoder->getDecodeFilePath() != playUrl) {
        videoDecoder->stopDecode();
        videoDecoder = std::make_shared<VideoDecoder>();
        videoDecoder->setVideoDecodeCallback(shared_from_this());
    }
    videoDecoder->decodeFile(playUrl);
}

void MediaPlayer::setPlayWindow(ANativeWindow *nativeWindow) {
    if(this->nativeWindow != nullptr) {
        ANativeWindow_release(this->nativeWindow);
    }
    this->nativeWindow = nativeWindow;
    ANativeWindow_acquire(nativeWindow);
}

void MediaPlayer::release() {
    videoDecoder->stopDecode();
    if(this->nativeWindow != nullptr) {
        ANativeWindow_release(this->nativeWindow);
        this->nativeWindow = nullptr;
    }
}

MediaPlayer::MediaPlayer() {
    videoDecoder = std::make_shared<VideoDecoder>();
}

MediaPlayer::~MediaPlayer() {
    release();
}

void MediaPlayer::playVideoFrame(AVFrame *avFrame) {
    //transform data to RGBA
    SwsContext *context = sws_getContext(avFrame->width, avFrame->height, static_cast<AVPixelFormat>(avFrame->format),
                                         avFrame->width, avFrame->height, AV_PIX_FMT_RGBA, 0,
                                         nullptr, nullptr, nullptr);
    AVFrame *dstFrame = av_frame_alloc();
    dstFrame->width = avFrame->width;
    dstFrame->height = avFrame->height;
    dstFrame->format = AV_PIX_FMT_RGBA;

    av_frame_get_buffer(dstFrame, 0);
    int ret = sws_scale_frame(context, dstFrame, avFrame);
    if(ret >= 0) {
        av_frame_copy_props(dstFrame, avFrame);
        ANativeWindow_Buffer buffer;
        ret = ANativeWindow_lock(nativeWindow, &buffer, nullptr);
        if(ret == 0) {
            uint8_t *dstBuffer = (uint8_t *)buffer.bits;
            int dstStride = buffer.stride * 4;
            int srcStride = dstFrame->linesize[0];
            for (int h = 0; h < dstFrame->height; h++) {
                memcpy(dstBuffer + h * dstStride, dstFrame->data[0] + h * srcStride, srcStride);
            }
            ret = ANativeWindow_unlockAndPost(nativeWindow);
            //log(LOG_TAG, "drawToWindow", avFrame->pts);
        }
    }
    av_frame_free(&dstFrame);
    sws_freeContext(context);
}

void MediaPlayer::playAudioFrame(AVFrame *avFrame) {

}

void MediaPlayer::onDecodeMetaData(DecodeMetaData data) {
    if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
        int ret = ANativeWindow_setBuffersGeometry(nativeWindow, data.w, data.h, WINDOW_FORMAT_RGBA_8888);
        log(LOG_TAG, "ANativeWindow_setBuffersGeometry", ret);
    }
}

void MediaPlayer::onDecodeFrameData(DecodeFrameData data) {
    if(data.isFinish) {
        return;
    }
    if(!videoDecoder->isDecoding()) {
        return;
    }
    AVFrame *avFrame = data.avFrame;
    if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
        playVideoFrame(avFrame);
    } else if (data.mediaType == AVMEDIA_TYPE_AUDIO) {
        playAudioFrame(avFrame);
    }
}
