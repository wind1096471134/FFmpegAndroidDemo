//
// Created by 10964 on 2024/11/7.
//

#ifndef FFMPEGDEMO_VIDEOENCODER_H
#define FFMPEGDEMO_VIDEOENCODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/codec.h"
#include "libavformat/avformat.h"
}

class VideoEncoder {
private:
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *avCodecContext = nullptr;
    const AVCodec *codec = nullptr;
public:
    VideoEncoder();
    int encodeImgToVideo(const AVFrame *imgFrame, const char *outputFile);
    void destroy();
};

#endif //FFMPEGDEMO_VIDEOENCODER_H