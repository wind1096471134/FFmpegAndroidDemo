//
// Created by allan on 2024/12/12.
//

#include "MediaAVSync.h"
#include "Util.h"
#include "thread"

MediaAVSync::MediaAVSync(): videoFrames(30), audioFrames(30) {

}

MediaAVSync::~MediaAVSync() {
    clear();
}

AVFrame *MediaAVSync::getNextVideoFrameOut() {
    AVFrame *frame = videoFrames.dequeue();
    if(frame == nullptr) {
        return nullptr;
    }
    //calculate and delay render if need.
    auto curTimestamp = getCurTimestamp();
    long long realShowDiff = curTimestamp - videoLastShowTimestamp;
    int perfectPts = frame->pts;
    //video and audio sync, video try to follow audio.
    if(audioLastPts > 0) {
        // transform lastAudioPts to time base on video
        int64_t audioPtsBaseVideo = rescaleTimestamp(audioLastPts, audioTimeBase, frame->time_base);
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
    return frame;
}

AVFrame *MediaAVSync::getNextAudioFrameOut() {
    AVFrame *frame = audioFrames.dequeue();
    if(frame == nullptr) {
        return nullptr;
    }
    //calculate and delay render if need.
    auto curTimestamp = getCurTimestamp();
    long long realShowDiff = curTimestamp - audioLastShowTimestamp;
    int64_t ptsDiff = frame->pts - audioLastPts;
    double ptsDuration = 1000 * av_q2d(av_mul_q({static_cast<int>(ptsDiff), 1}, frame->time_base));
    int delay = ptsDuration - realShowDiff;
    if(delay > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    return frame;
}

void MediaAVSync::enqueueVideoFrameIn(AVFrame *frame) {
    videoFrames.enqueue(frame);
}

void MediaAVSync::enqueueAudioFrameIn(AVFrame *frame) {
    audioFrames.enqueue(frame);
    if(this->audioTimeBase.load().num == 0) {
        this->audioTimeBase = frame->time_base;
    }
}

void MediaAVSync::clear() {
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
    videoLastPts = 0;
    videoLastShowTimestamp = 0;
    audioLastPts = 0;
    audioLastShowTimestamp = 0;
    audioTimeBase = {0,1};
}

void MediaAVSync::markVideoFrameShow(int64_t framePts) {
    videoLastPts = framePts;
    videoLastShowTimestamp = getCurTimestamp();
}

void MediaAVSync::markAudioFrameShow(int64_t framePts) {
    audioLastPts = framePts;
    audioLastShowTimestamp = getCurTimestamp();
}
