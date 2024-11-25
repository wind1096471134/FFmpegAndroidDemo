//
// Created by allan on 2024/11/24.
//

#ifndef FFMPEGDEMO_BASEDECODER_H
#define FFMPEGDEMO_BASEDECODER_H

extern "C"{
#include "libavcodec/codec.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
#include "libavformat/avformat.h"
}
#include "functional"

struct DecodeFrameData {
    int frameIndex; //[0,n]
    AVMediaType mediaType; //1:Video; 2:Audio
    AVFrame *avFrame;
};
using DecodeVideoCallback = std::function<void(DecodeFrameData &data)>;

class BaseDecoder{
protected:
    AVFormatContext *avFormatContext = nullptr;
    AVCodecContext  *avCodecContext = nullptr;
    const AVCodec *avCodec = nullptr;
    const AVMediaType mediaType;

    void freeResource();
public:
    BaseDecoder(AVMediaType avMediaType): mediaType(avMediaType) {};
    int decodeFile(const char *filePath, DecodeVideoCallback &decodeCallback);
    ~BaseDecoder();
};

#endif //FFMPEGDEMO_BASEDECODER_H
