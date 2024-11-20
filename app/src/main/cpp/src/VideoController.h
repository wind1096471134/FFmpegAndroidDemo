//
// Created by allan on 2024/11/16.
//

#ifndef FFMPEGDEMO_VIDEOCONTROLLER_H
#define FFMPEGDEMO_VIDEOCONTROLLER_H

#include "VideoDecoder.h"
#include "VideoEncoder.h"

class VideoController{
public:
    VideoController();

    int encodeVideoWithImg(const char *imgInputPath, const char *videoOutputPath);
};
#endif //FFMPEGDEMO_VIDEOCONTROLLER_H
