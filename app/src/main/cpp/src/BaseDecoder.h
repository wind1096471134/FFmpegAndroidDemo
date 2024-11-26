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
#include "string"

struct DecodeFrameData {
    int frameIndex; //[0,n]
    AVMediaType mediaType; //1:Video; 2:Audio
    AVFrame *avFrame;
    bool isFinish;
};
using DecodeVideoCallback = std::function<void(DecodeFrameData data)>;

class BaseDecoder{
protected:
    AVFormatContext *avFormatContext = nullptr;
    AVCodecContext  *avCodecContext = nullptr;
    const AVCodec *avCodec = nullptr;
    const AVMediaType mediaType;
    std::string inputFilePath;

    void freeResource();
public:
    BaseDecoder(AVMediaType avMediaType): mediaType(avMediaType) {};
    int decodeFile(const std::string &filePath, DecodeVideoCallback &decodeCallback);
    ~BaseDecoder();
};

#endif //FFMPEGDEMO_BASEDECODER_H
