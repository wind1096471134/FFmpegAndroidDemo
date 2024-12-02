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
#include "atomic"
#include "BlockingQueue.h"

struct VideoEncodeParam {
    bool encode;
    int bitRate;
    int w;
    int h;
    int fps;
};
struct AudioEncodeParam {
    bool encode;
    int bitRate;
    int sampleRate;
    AVChannelLayout channelLayout;
};
struct EncodeFrame {
    AVFrame *avFrame;
    AVMediaType mediaType;
};
class IEncodeCallback {
public:
    virtual ~IEncodeCallback() = default;
    virtual void onEncodeStart() {}
    virtual void onEncodeFinish(int ret, const std::string &encodeFile) {}
};

class VideoEncoder {
private:
    AVFormatContext *outputFormatContext = nullptr;
    AVCodecContext *videoCodecContext = nullptr;
    AVStream *videoStream = nullptr;
    const AVCodec *videoCodec = nullptr;
    AVCodecContext *audioCodecContext = nullptr;
    AVStream *audioStream = nullptr;
    const AVCodec *audioCodec = nullptr;
    std::atomic<VideoEncodeParam> videoEncodeParam;
    std::atomic<AudioEncodeParam> audioEncodeParam;
    std::atomic<int> videoFrameInputNum;
    std::atomic<int> audioFrameInputSampleNum;
    std::atomic<int> videoFramePts;
    std::atomic<int> audioFramePts;
    std::atomic<bool> decodeRunning;
    BlockingQueue<EncodeFrame> queue;
    std::shared_ptr<IEncodeCallback> encodeCallback = nullptr;

    void freeResource();
    int encodeThreadHandlerLoop(const std::string &outputFile, const VideoEncodeParam &videoEncodeParam, const AudioEncodeParam &audioEncodeParam);
    int encodeFrameInternal(const EncodeFrame &encodeFrame);

public:
    VideoEncoder();
    void setEncodeCallback(std::shared_ptr<IEncodeCallback> encodeCallback);
    //编码开始，输入数据前必须调用一次
    int encodeStart(const std::string &outputFile, const VideoEncodeParam &videoEncodeParam, const AudioEncodeParam &audioEncodeParam);
    //输入每一帧数据
    int encodeFrame(const EncodeFrame &encodeFrame);
    //编码结束，数据输入结束必须调用一次
    int encodeEnd();
    //获取已编码视频流时长（s），非精确值。
    int getEncodeVideoDuration();
    //获取已编码音频流时长（s），非精确值。
    int getEncodeAudioDuration();
    ~VideoEncoder();
};
#endif //FFMPEGDEMO_VIDEOENCODER_H