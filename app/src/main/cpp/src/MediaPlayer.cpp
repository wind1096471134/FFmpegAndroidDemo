//
// Created by allan on 2024/12/4.
//

#include "MediaPlayer.h"
#include "Util.h"
#include "thread"
#include "chrono"
extern "C" {
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#define LOG_TAG "MediaPlayer"

void MediaPlayer::play(std::string& playUrl) {
    if(playStatus == PLAY) {
        return;
    }
    videoDecoder->setVideoDecodeCallback(shared_from_this());
    videoDecoder->decodeFile(playUrl);
    playStatus = PLAY;
    //render thread
    std::thread videoRenderThread([&]() {
        log(LOG_TAG, "videoRenderThread start");
        try{
            while(playStatus == PLAY) {
                AVFrame *frame = videoFrames.dequeue();
                //calculate and delay render if need.
                auto curTimestamp = getCurTimestamp();
                long long realShowDiff = curTimestamp - videoLastShowTimestamp;
                int64_t ptsDiff = frame->pts - videoLastPts;
                double ptsDuration = 1000 * av_q2d(av_mul_q({static_cast<int>(ptsDiff), 1}, frame->time_base));
                int delay = ptsDuration - realShowDiff;

                if(delay > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                }

                ANativeWindow_Buffer buffer;
                int ret = ANativeWindow_lock(nativeWindow, &buffer, nullptr);
                if(ret == 0) {
                    uint8_t *dstBuffer = (uint8_t *) buffer.bits;
                    int dstStride = buffer.stride * 4;
                    int srcStride = frame->linesize[0];
                    for (int h = 0; h < frame->height; h++) {
                        memcpy(dstBuffer + h * dstStride, frame->data[0] + h * srcStride, srcStride);
                    }
                    ret = ANativeWindow_unlockAndPost(nativeWindow);
                }

                videoLastPts = frame->pts;
                videoLastShowTimestamp = getCurTimestamp();
                av_frame_free(&frame);
            }
        } catch (const std::runtime_error& e) {
            log(LOG_TAG, e.what());
        }

        log(LOG_TAG, "videoRenderThread end");
    });
    videoRenderThread.detach();
}

void MediaPlayer::setPlayWindow(ANativeWindow *nativeWindow) {
    if(this->nativeWindow != nullptr) {
        ANativeWindow_release(this->nativeWindow);
    }
    this->nativeWindow = nativeWindow;
    ANativeWindow_acquire(nativeWindow);
}

void MediaPlayer::release() {
    clearData();
    if(this->nativeWindow != nullptr) {
        ANativeWindow_release(this->nativeWindow);
        this->nativeWindow = nullptr;
    }
}

void MediaPlayer::clearData() {
    videoDecoder->stopDecode();
    playStatus = DESTROY;
    while(!videoFrames.isEmpty()) {
        AVFrame *frame = videoFrames.dequeue();
        av_frame_free(&frame);
    }
    videoFrames.shutdown();
    videoLastPts = 0;
    videoLastShowTimestamp = 0;
}

MediaPlayer::MediaPlayer(): videoFrames(30), playStatus(INIT){
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
        if(dstFrame->best_effort_timestamp > 0) {
            dstFrame->pts = dstFrame->best_effort_timestamp;
        }
        videoFrames.enqueue(dstFrame);
    }
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
