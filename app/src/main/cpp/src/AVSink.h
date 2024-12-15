//
// Created by allan on 2024/12/14.
//

#ifndef FFMPEGDEMO_AVSINK_H
#define FFMPEGDEMO_AVSINK_H

extern "C" {
#include "libavutil/frame.h"
}

/**
 * sink to display frame on surface/audioTrack...
 */
class AVSink {
public:
    virtual void processFrame(AVFrame *avFrame) = 0;
    virtual void release() = 0;
};

#endif //FFMPEGDEMO_AVSINK_H
