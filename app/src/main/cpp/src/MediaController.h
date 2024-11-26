//
// Created by allan on 2024/11/16.
//

#ifndef FFMPEGDEMO_MEDIACONTROLLER_H
#define FFMPEGDEMO_MEDIACONTROLLER_H

#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "AudioDecoder.h"
#include "string"

class MediaController{
private:
    VideoDecoder *videoDecoder = nullptr;
    AudioDecoder *audioDecoder = nullptr;
    VideoEncoder *videoEncoder = nullptr;
    std::string videoOutputPath;
    DecodeVideoCallback mediaDecodeFrameCallback = [&](DecodeFrameData data) {
        onDecodeFrameCallback(data);
    };
    void onDecodeFrameCallback(DecodeFrameData &data);
public:
    MediaController();
    ~MediaController();
    int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath);
    int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                                 const std::string &videoOutputPath);
};

//Tools method.
int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath);
int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                             const std::string &videoOutputPath);

#endif //FFMPEGDEMO_MEDIACONTROLLER_H
