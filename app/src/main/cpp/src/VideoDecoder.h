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

//frame data
struct DecodeFrameData {
    AVMediaType mediaType; //1:Video; 2:Audio
    AVFrame *avFrame;
    bool isFinish; //if true, avFrame is null
};
//meta data
struct DecodeMetaData {
    AVMediaType mediaType;
    //for video
    int w;
    int h;
    int fps; //avg value
    //for audio
    int sampleRate;
    AVChannelLayout channelLayout;
    AVSampleFormat sampleFormat;
};

class IVideoDecodeCallback {
public:
    virtual ~IVideoDecodeCallback() = default;
    virtual void onDecodeMetaData(DecodeMetaData data) = 0;
    virtual void onDecodeFrameData(DecodeFrameData data) = 0;
};

class VideoDecoder{
protected:
    AVFormatContext *avFormatContext = nullptr;
    AVCodecContext  *videoCodecContext = nullptr;
    const AVCodec *videoCodec = nullptr;
    int videoStreamId = -1;
    AVCodecContext  *audioCodecContext = nullptr;
    const AVCodec *audioCodec = nullptr;
    std::string filePath;
    int audioStreamId = -1;
    std::atomic<bool> decoding;
    std::shared_ptr<IVideoDecodeCallback> videoDecodeCallback = nullptr;
    bool hasCallbackVideoMetaData = false;
    bool hasCallbackAudioMetaData = false;

    int initCodec(AVMediaType mediaType, AVCodecContext *&avCodecContext, const AVCodec *&avCodec, int &streamId);
public:
    VideoDecoder();
    void setVideoDecodeCallback(std::shared_ptr<IVideoDecodeCallback> videoDecodeCallback);
    int decodeFile(const std::string &inputFilePath);
    void stopDecode();
    bool isDecoding();
    std::string& getDecodeFilePath();
    void freeResource();
    ~VideoDecoder();
};

#endif //FFMPEGDEMO_VIDEODECODER_H
