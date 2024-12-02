//
// Created by allan on 2024/11/24.
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
#include "string"

struct DecodeFrameData {
    int frameIndex; //[0,n]
    AVMediaType mediaType; //1:Video; 2:Audio
    AVFrame *avFrame;
    bool isFinish;
};
using DecodeVideoCallback = std::function<void(DecodeFrameData data)>;

class VideoDecoder{
protected:
    AVFormatContext *avFormatContext = nullptr;
    AVCodecContext  *videoCodecContext = nullptr;
    const AVCodec *videoCodec = nullptr;
    int videoStreamId = -1;
    AVCodecContext  *audioCodecContext = nullptr;
    const AVCodec *audioCodec = nullptr;
    int audioStreamId = -1;
    std::atomic<bool> decoding;

    void freeResource();
    int initCodec(AVMediaType mediaType, AVCodecContext *&avCodecContext, const AVCodec *&avCodec, int &streamId);
public:
    VideoDecoder();
    int decodeFile(const std::string &inputFilePath, DecodeVideoCallback &decodeCallback);
    void stopDecode();
    ~VideoDecoder();
};

#endif //FFMPEGDEMO_VIDEODECODER_H
