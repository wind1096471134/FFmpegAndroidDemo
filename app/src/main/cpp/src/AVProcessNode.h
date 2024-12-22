//
// Created by allan on 2024/12/14.
//

#ifndef FFMPEGDEMO_AVPROCESSNODE_H
#define FFMPEGDEMO_AVPROCESSNODE_H

extern "C" {
#include "libavutil/frame.h"
}

/**
 * Process node to consume a frame and output a new frame.
 */
class AVProcessNode {
public:
    virtual AVFrame* processFrame(AVFrame *avFrame) = 0;
    virtual void release() = 0;
};

#endif //FFMPEGDEMO_AVPROCESSNODE_H
