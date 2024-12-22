//
// Created by allan on 2024/12/22.
//

#ifndef FFMPEGDEMO_AVPROCESSSINK_H
#define FFMPEGDEMO_AVPROCESSSINK_H

extern "C" {
#include "libavutil/frame.h"
}

/**
 * Process node to consume a frame and output a new frame.
 */
class AVProcessSink {
public:
    virtual void processFrame(AVFrame *avFrame) = 0;
    virtual void release() = 0;
};

#endif //FFMPEGDEMO_AVPROCESSSINK_H
