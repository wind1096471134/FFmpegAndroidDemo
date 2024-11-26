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
#include "string"

struct VideoEncodeParam {
    int w;
    int h;
    int fps;
    int bitRate;
};

class VideoEncoder {
private:
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *avCodecContext = nullptr;
    AVStream *avStream = nullptr;
    const AVCodec *codec = nullptr;
    int frameCount = 0;
    void freeResource();

public:
    VideoEncoder();
    //编码开始，输入数据前必须调用一次
    int encodeStart(const std::string &outputFile, VideoEncodeParam &param);
    //输入每一帧数据
    int encodeFrame(const AVFrame *avFrame);
    //编码结束，数据输入结束必须调用一次
    int encodeEnd();
    ~VideoEncoder();
};

//tools method
int videoEncoder_encodeImgToVideo(const AVFrame *avFrame, const std::string &outputFile, VideoEncodeParam &param, int durationSecond);

#endif //FFMPEGDEMO_VIDEOENCODER_H