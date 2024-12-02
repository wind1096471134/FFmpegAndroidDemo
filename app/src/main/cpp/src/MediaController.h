//
// Created by allan on 2024/11/16.
//

#ifndef FFMPEGDEMO_MEDIACONTROLLER_H
#define FFMPEGDEMO_MEDIACONTROLLER_H

#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "AudioDecoder.h"
#include "string"

#define DEFAULT_VIDEO_FPS 25
#define DEFAULT_VIDEO_DURATION 5

class MediaController: public IEncodeCallback, public std::enable_shared_from_this<MediaController>{
private:
    std::shared_ptr<VideoDecoder> videoDecoder = nullptr;
    std::shared_ptr<AudioDecoder> audioDecoder = nullptr;
    std::shared_ptr<VideoEncoder> videoEncoder = nullptr;
    bool videoDecodeEnd = false;
    bool audioDecodeEnd = false;
    int encodeDuration = DEFAULT_VIDEO_DURATION;
    int encodeFps = DEFAULT_VIDEO_FPS;
    int repeatVideoFrameNum = 1;
    DecodeVideoCallback mediaDecodeFrameCallback = [&](DecodeFrameData data) {
        onDecodeFrameCallback(data);
    };
    std::shared_ptr<IEncodeCallback> outsideEncodeCallback = nullptr;
    void onDecodeFrameCallback(DecodeFrameData &data);
    void onEncodeStart() override;
    void onEncodeFinish(int ret, const std::string &encodeFile) override;
public:
    MediaController();
    ~MediaController() override;
    void setEncodeCallback(std::shared_ptr<IEncodeCallback> callback);
    int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath);
    int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                                 const std::string &videoOutputPath);
};

#endif //FFMPEGDEMO_MEDIACONTROLLER_H
