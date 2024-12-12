//
// Created by allan on 2024/12/4.
//

#ifndef FFMPEGDEMO_MEDIAPLAYER_H
#define FFMPEGDEMO_MEDIAPLAYER_H

#include "android/native_window_jni.h"
#include "string"
#include "VideoDecoder.h"
#include "BlockingQueue.h"
#include "atomic"
#include "NativeAudioTrackWrapper.h"
#include "MediaAVSync.h"

enum PlayState {
    INIT,
    PLAY,
    PAUSE,
    DESTROY
};

class IPlayerStateCallback {
public:
    virtual ~IPlayerStateCallback() = default;
    virtual void onStateChange(PlayState state) = 0;
};

class MediaPlayer: public IVideoDecodeCallback, public std::enable_shared_from_this<MediaPlayer>{
private:
    ANativeWindow *nativeWindow = nullptr;
    std::shared_ptr<VideoDecoder> videoDecoder = nullptr;
    std::shared_ptr<IVideoDecodeCallback> mediaDecodeCallback = nullptr;
    std::shared_ptr<NativeAudioTrackWrapper> audioTrack = nullptr;
    std::shared_ptr<IPlayerStateCallback> playerStateCallback = nullptr;
    std::atomic<PlayState> playState;
    AVSampleFormat targetSampleFormat;
    AVChannelLayout targetChLayout{};
    std::mutex playStateMutex;
    std::condition_variable playStateCondition;
    MediaAVSync mediaAvSync;

    void setState(PlayState playStatus);
    void onDecodeMetaData(DecodeMetaData data) override;
    void onDecodeFrameData(DecodeFrameData data) override;
    void playVideoFrame(AVFrame *avFrame);
    void playAudioFrame(AVFrame *avFrame);
    void clearData();
    void waitUntilPlay();
public:
    MediaPlayer();
    ~MediaPlayer() override;
    void play(std::string& playUrl);
    void pause();
    void resume();
    void setPlayWindow(ANativeWindow *nativeWindow);
    void setAudioTrack(std::shared_ptr<NativeAudioTrackWrapper> audioTrackPtr);
    void setPlayerStateCallback(std::shared_ptr<IPlayerStateCallback> playerStateCallback);
    void release();
};

#endif //FFMPEGDEMO_MEDIAPLAYER_H
