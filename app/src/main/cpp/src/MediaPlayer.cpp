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
        //avoid outside class destroy early.
        std::shared_ptr<MediaPlayer> mediaPlayer = shared_from_this();
        while(playState != DESTROY) {
            mediaAvSync->syncAndPlayNextVideoFrame(videoSink.get());
            waitUntilPlay();
        }
        videoSink->release();
        log(LOG_TAG, "videoRenderThread end", mediaPlayer.use_count());
    });
    videoRenderThread.detach();

    //audio thread
    std::thread audioPlayThread([&]() {
        log(LOG_TAG, "audioPlayThread start");
        //avoid outside class destroy early.
        std::shared_ptr<MediaPlayer> mediaPlayer = shared_from_this();
        while(playState != DESTROY) {
            mediaAvSync->syncAndPlayNextAudioFrame(audioSink.get());
            waitUntilPlay();
        }
        audioSink->release();
        log(LOG_TAG, "audioPlayThread end", mediaPlayer.use_count());
    });
    audioPlayThread.detach();
}

void MediaPlayer::release() {
    clearData();
}

void MediaPlayer::clearData() {
    if(videoDecoder != nullptr) {
        videoDecoder->stopDecode();
        videoDecoder = nullptr;
    }
    setState(DESTROY);
    mediaAvSync->clear();
}

MediaPlayer::MediaPlayer(ANativeWindow *nativeWindow, std::shared_ptr<NativeAudioTrackWrapper> audioTrackWrapper):
        playState(INIT), playStateMutex(), playStateCondition() {
    videoDecoder = std::make_shared<VideoDecoder>();
    videoSink = std::make_shared<VideoSurfaceSink>(nativeWindow);
    audioSink = std::make_shared<AudioTrackSink>(audioTrackWrapper);
    mediaAvSync = std::make_shared<MediaAVSync>();
}

MediaPlayer::~MediaPlayer() {
    log(LOG_TAG, "~MediaPlayer");
}

void MediaPlayer::sendVideoFrame(AVFrame *avFrame) {
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
    sws_freeContext(context);
    if(ret >= 0) {
        av_frame_copy_props(dstFrame, avFrame);
        if(dstFrame->best_effort_timestamp > 0) {
            dstFrame->pts = dstFrame->best_effort_timestamp;
        }
        mediaAvSync->enqueueVideoFrameIn(dstFrame);
    }
}

void MediaPlayer::sendAudioFrame(AVFrame *avFrame) {
    AVFrame *dstFrame = avFrame;
    //transform audio format
    if(avFrame->format != targetSampleFormat ||
       av_channel_layout_compare(&targetChLayout, &(avFrame->ch_layout)) != 0) {
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

    mediaAvSync->enqueueAudioFrameIn(dstFrame);
}

void MediaPlayer::onDecodeMetaData(DecodeMetaData data) {
    if(data.mediaType == AVMEDIA_TYPE_VIDEO) {
        videoSink->setSurfaceSize(data.w, data.h);
    } else if (data.mediaType == AVMEDIA_TYPE_AUDIO) {
        const AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        const AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
        if (av_channel_layout_compare(&stereo, &(data.channelLayout)) == 0) {
            targetChLayout = stereo;
        } else if (av_channel_layout_compare(&mono, &(data.channelLayout)) == 0) {
            targetChLayout = mono;
        } else {
            targetChLayout = stereo;
        }
        //android support AV_SAMPLE_FMT_S16 well.
        targetSampleFormat = AV_SAMPLE_FMT_S16;

        audioSink->setAudioConfig(data.sampleRate, targetChLayout, targetSampleFormat);
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
        sendVideoFrame(avFrame);
    } else if (data.mediaType == AVMEDIA_TYPE_AUDIO) {
        sendAudioFrame(avFrame);
    }
    waitUntilPlay();
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
