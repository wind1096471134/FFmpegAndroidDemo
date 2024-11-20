//
// Created by allan on 2024/11/14.
//

#ifndef FFMPEGDEMO_VIDEODECODER_H
#define FFMPEGDEMO_VIDEODECODER_H

extern "C"{
#include "libavcodec/codec.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
#include "libavformat/avformat.h"
}
#include "functional"

struct DecodeFrameData {
    int frameIndex; //[0,n]
    int mediaType; //1:Video; 2:Audio
    AVFrame *avFrame;
};
using DecodeVideoCallback = std::function<void(DecodeFrameData &data)>;

class VideoDecoder{
private:
    AVFormatContext *avFormatContext = nullptr;
    AVCodecContext  *avCodecContext = nullptr;
    const AVCodec *avCodec = nullptr;

    int decodeVideo(const char *filePath, DecodeVideoCallback &decodeCallback);
public:
    VideoDecoder();
    int decodeFile(const char *filePath, DecodeVideoCallback &decodeCallback);
    void destroy();
};

#endif //FFMPEGDEMO_VIDEODECODER_H
