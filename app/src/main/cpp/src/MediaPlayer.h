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
#include "VideoSurfaceSink.h"
#include "AudioTrackSink.h"

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
    std::mutex videoDecoderMutex;
    std::shared_ptr<VideoDecoder> videoDecoder = nullptr;
    std::shared_ptr<IVideoDecodeCallback> mediaDecodeCallback = nullptr;
    std::shared_ptr<IPlayerStateCallback> playerStateCallback = nullptr;
    std::atomic<PlayState> playState;
    AVSampleFormat targetSampleFormat;
    AVChannelLayout targetChLayout{};
    std::mutex playStateMutex;
    std::condition_variable playStateCondition;
    std::shared_ptr<MediaAVSync> mediaAvSync;
    std::shared_ptr<VideoSurfaceSink> videoSink;
    std::shared_ptr<AudioTrackSink> audioSink;
    std::atomic<bool> isLoop;
    std::string playUrl;

    void setState(PlayState playStatus);
    void onDecodeMetaData(DecodeMetaData data) override;
    void onDecodeFrameData(DecodeFrameData data) override;
    void onDecodeEnd() override;
    void sendVideoFrame(AVFrame *avFrame);
    void sendAudioFrame(AVFrame *avFrame);
    void clearData();
    void waitUntilPlay();
    void startDecoder(std::string &playUrl);
public:
    MediaPlayer(ANativeWindow *nativeWindow, std::shared_ptr<NativeAudioTrackWrapper> audioTrackWrapper);
    ~MediaPlayer() override;
    void play(std::string& playUrl);
    void pause();
    void resume();
    void setLoop(bool loop);
    void setPlayerStateCallback(std::shared_ptr<IPlayerStateCallback> playerStateCallback);
    void release();
};

#endif //FFMPEGDEMO_MEDIAPLAYER_H
