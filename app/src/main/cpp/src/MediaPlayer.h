//
// Created by allan on 2024/12/4.
//

#ifndef FFMPEGDEMO_MEDIAPLAYER_H
#define FFMPEGDEMO_MEDIAPLAYER_H

#include "android/native_window_jni.h"
#include "string"
#include "VideoDecoder.h"

class MediaPlayer: public IVideoDecodeCallback, public std::enable_shared_from_this<MediaPlayer>{
private:
    ANativeWindow *nativeWindow = nullptr;
    std::shared_ptr<VideoDecoder> videoDecoder = nullptr;
    std::shared_ptr<IVideoDecodeCallback> mediaDecodeCallback = nullptr;
    void onDecodeMetaData(DecodeMetaData data) override;
    void onDecodeFrameData(DecodeFrameData data) override;
    void playVideoFrame(AVFrame *avFrame);
    void playAudioFrame(AVFrame *avFrame);
public:
    MediaPlayer();
    ~MediaPlayer() override;
    void play(std::string& playUrl);
    void setPlayWindow(ANativeWindow *nativeWindow);
    void release();
};

#endif //FFMPEGDEMO_MEDIAPLAYER_H
