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
    if(playState == PLAY) {
        return;
    }
    videoDecoder->setVideoDecodeCallback(shared_from_this());
    videoDecoder->decodeFile(playUrl);
    setState(PLAY);
    //video render thread
    std::thread videoRenderThread([&]() {
        log(LOG_TAG, "videoRenderThread start");
        try{
            int64_t videoLastPts = 0;
            long long videoLastShowTimestamp = 0;
            while(playState != DESTROY) {
                AVFrame *frame = videoFrames.dequeue();
                if(frame == nullptr) {
                    continue;
                }
                //calculate and delay render if need.
                auto curTimestamp = getCurTimestamp();
                long long realShowDiff = curTimestamp - videoLastShowTimestamp;
                int perfectPts = frame->pts;
                //video and audio sync, video try to follow audio.
                if(lastAudioPlayPts > 0) {
                    // transform lastAudioPts to time base on video
                    int64_t audioPtsBaseVideo = rescaleTimestamp(lastAudioPlayPts, audioTimeBase, frame->time_base);
                    // if(audioPtsBaseVideo > videoLastPts) video slow than audio else video fast than audio
                    int avDiff = videoLastPts - audioPtsBaseVideo;
                    perfectPts += avDiff;
                }
                int64_t ptsDiff = perfectPts - videoLastPts;
                //transform ptsDiff to real time diff (ms)
                double realTimePtsDiff = 1000 * av_q2d(av_mul_q({static_cast<int>(ptsDiff), 1}, frame->time_base));
                int delay = realTimePtsDiff - realShowDiff;
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

                waitUntilPlay();
            }
        } catch (const std::exception& e) { //not work?
            log(LOG_TAG , e.what());
        }

        log(LOG_TAG, "videoRenderThread end");
    });
    videoRenderThread.detach();

    //audio thread
    std::thread audioPlayThread([&]() {
        if(audioTrack == nullptr) {
            return ;
        }
        log(LOG_TAG, "audioPlayThread start");

        int64_t audioLastPts = 0;
        long long audioLastShowTimestamp = 0;
        while(playState != DESTROY) {
            AVFrame *avFrame = audioFrames.dequeue();
            if(avFrame == nullptr) {
                continue;
            }
            //calculate and delay render if need.
            auto curTimestamp = getCurTimestamp();
            long long realShowDiff = curTimestamp - audioLastShowTimestamp;
            int64_t ptsDiff = avFrame->pts - audioLastPts;
            double ptsDuration = 1000 * av_q2d(av_mul_q({static_cast<int>(ptsDiff), 1}, avFrame->time_base));
            int delay = ptsDuration - realShowDiff;
            if(delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            this->lastAudioPlayPts = avFrame->pts;
            this->audioTimeBase = avFrame->time_base;

            audioTrack->playFrame(avFrame->data[0], avFrame->linesize[0]);

            audioLastPts = avFrame->pts;
            audioLastShowTimestamp = getCurTimestamp();

            av_frame_free(&avFrame);

            waitUntilPlay();
        }

        log(LOG_TAG, "audioPlayThread end");
    });
    audioPlayThread.detach();
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
    if(audioTrack != nullptr) {
        audioTrack->playEnd();
    }
}

void MediaPlayer::clearData() {
    videoDecoder->stopDecode();
    setState(DESTROY);
    while(!videoFrames.isEmpty()) {
        AVFrame *frame = videoFrames.dequeue();
        av_frame_free(&frame);
    }
    videoFrames.shutdown();
    while(!audioFrames.isEmpty()) {
        AVFrame *frame = audioFrames.dequeue();
        av_frame_free(&frame);
    }
    audioFrames.shutdown();
}

MediaPlayer::MediaPlayer(): videoFrames(30), playState(INIT), lastAudioPlayPts(0), audioTimeBase({1, 1}), playStateMutex(), playStateCondition(){
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
    if(audioTrack != nullptr) {
        AVFrame *dstFrame = avFrame;
        if(avFrame->format != targetSampleFormat ||
                av_channel_layout_compare(&targetChLayout, &(avFrame->ch_layout)) != 0) {
            //transform audio format
            SwrContext *swrContext = nullptr;
            int ret = swr_alloc_set_opts2(&swrContext, &targetChLayout, targetSampleFormat, avFrame->sample_rate,
                                          &avFrame->ch_layout, static_cast<AVSampleFormat>(avFrame->format), avFrame->sample_rate, 0,
                                          nullptr);
            if(ret < 0 || swr_init(swrContext) < 0) {
                log(LOG_TAG, "swr_alloc_set_opts2 fail", ret);
                return;
            }
            AVFrame *audioFrame = av_frame_alloc();
            audioFrame->ch_layout = targetChLayout;
            audioFrame->format = targetSampleFormat;
            audioFrame->sample_rate = avFrame->sample_rate;
            audioFrame->time_base = avFrame->time_base;

            av_frame_get_buffer(audioFrame, 0);
            ret = swr_convert_frame(swrContext, audioFrame, avFrame);
            swr_free(&swrContext);
            if(ret < 0) {
                log(LOG_TAG, "swr_alloc_set_opts2 fail", ret);
                return;
            }
            dstFrame = audioFrame;
        } else {
            AVFrame *audioFrame = av_frame_clone(avFrame);
            av_frame_copy_props(audioFrame, avFrame);
            dstFrame = audioFrame;
        }
        dstFrame->pts = avFrame->pts;
        dstFrame->best_effort_timestamp = avFrame->best_effort_timestamp;
        if(dstFrame->best_effort_timestamp > 0) {
            dstFrame->pts = dstFrame->best_effort_timestamp;
        }
        audioFrames.enqueue(dstFrame);
    }
}

void MediaPlayer::onDecodeMetaData(DecodeMetaData data) {
    if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
        int ret = ANativeWindow_setBuffersGeometry(nativeWindow, data.w, data.h, WINDOW_FORMAT_RGBA_8888);
        log(LOG_TAG, "ANativeWindow_setBuffersGeometry", ret);
    } else if (data.mediaType == AVMEDIA_TYPE_AUDIO) {
        if(audioTrack != nullptr) {
            int channelConfig;
            const AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            const AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
            if(av_channel_layout_compare(&stereo, &(data.channelLayout)) == 0) {
                channelConfig = 2;
                targetChLayout = stereo;
            } else if (av_channel_layout_compare(&mono, &(data.channelLayout)) == 0) {
                channelConfig = 1;
                targetChLayout = mono;
            } else {
                channelConfig = 2;
                targetChLayout = stereo;
            }
            //android support AV_SAMPLE_FMT_S16 well.
            int sampleFormat = 2;
            targetSampleFormat = AV_SAMPLE_FMT_S16;
            audioTrack->playStart(data.sampleRate, channelConfig, sampleFormat);
        }
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
    waitUntilPlay();
}

void MediaPlayer::setAudioTrack(std::shared_ptr<NativeAudioTrackWrapper> audioTrackPtr) {
    if(this->audioTrack != nullptr) {
        this->audioTrack->playEnd();
    }
    this->audioTrack = audioTrackPtr;
}

void MediaPlayer::setPlayerStateCallback(std::shared_ptr<IPlayerStateCallback> playerStateCallback) {
    this->playerStateCallback = playerStateCallback;
}

void MediaPlayer::setState(PlayState playStatus) {
    this->playState = playStatus;
    playStateCondition.notify_all();
    if(playerStateCallback != nullptr) {
        playerStateCallback->onStateChange(playStatus);
    }
}

void MediaPlayer::pause() {
    setState(PAUSE);
}

void MediaPlayer::resume() {
    setState(PLAY);
}

void MediaPlayer::waitUntilPlay() {
    std::unique_lock<std::mutex> lock(playStateMutex);
    playStateCondition.wait(lock, [this] {return playState != PAUSE;});
}
