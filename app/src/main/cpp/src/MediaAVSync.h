//
// Created by allan on 2024/12/12.
//

#ifndef FFMPEGDEMO_MEDIAAVSYNC_H
#define FFMPEGDEMO_MEDIAAVSYNC_H

#include "BlockingQueue.h"

extern "C"{
#include "libavutil/frame.h"
}

class MediaAVSync {
private:
    BlockingQueue<AVFrame*> videoFrames;
    BlockingQueue<AVFrame*> audioFrames;
    std::atomic<int64_t> videoLastPts{};
    std::atomic<long long> videoLastShowTimestamp{};
    std::atomic<int64_t> audioLastPts{};
    std::atomic<long long> audioLastShowTimestamp{};
    std::atomic<AVRational> audioTimeBase{};
public:
    MediaAVSync();
    ~MediaAVSync();
    AVFrame* getNextVideoFrameOut();
    AVFrame* getNextAudioFrameOut();
    void markVideoFrameShow(int64_t framePts);
    void markAudioFrameShow(int64_t framePts);
    void enqueueVideoFrameIn(AVFrame *frame);
    void enqueueAudioFrameIn(AVFrame *frame);
    void clear();
};
#endif //FFMPEGDEMO_MEDIAAVSYNC_H
