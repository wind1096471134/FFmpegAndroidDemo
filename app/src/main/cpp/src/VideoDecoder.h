//
// Created by allan on 2024/11/14.
//

#ifndef FFMPEGDEMO_VIDEODECODER_H
#define FFMPEGDEMO_VIDEODECODER_H

#include "BaseDecoder.h"

class VideoDecoder: public BaseDecoder {
public:
    VideoDecoder(): BaseDecoder(AVMEDIA_TYPE_VIDEO) {}
    ~VideoDecoder() = default;
};

#endif //FFMPEGDEMO_VIDEODECODER_H
