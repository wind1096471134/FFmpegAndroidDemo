//
// Created by allan on 2024/12/14.
//

#ifndef FFMPEGDEMO_AUDIOTRACKSINK_H
#define FFMPEGDEMO_AUDIOTRACKSINK_H

#include "AVProcessSink.h"
#include "NativeAudioTrackWrapper.h"

class AudioTrackSink: public AVProcessSink {
private:
    std::shared_ptr<NativeAudioTrackWrapper> audioTrack;
    void freeRes();
public:
    explicit AudioTrackSink(std::shared_ptr<NativeAudioTrackWrapper> audioTrack);
    ~AudioTrackSink();
    void setAudioConfig(int sampleRate, AVChannelLayout channelLayout, AVSampleFormat sampleFormat);
    void processFrame(AVFrame *avFrame) override;
    void release() override;
};

#endif //FFMPEGDEMO_AUDIOTRACKSINK_H
