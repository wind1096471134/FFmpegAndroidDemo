//
// Created by allan on 2024/11/16.
//

#ifndef FFMPEGDEMO_MEDIACONTROLLER_H
#define FFMPEGDEMO_MEDIACONTROLLER_H

#include "VideoDecoder.h"
#include "VideoEncoder.h"
#include "string"

#define DEFAULT_VIDEO_FPS 25
#define DEFAULT_VIDEO_DURATION 5000

class MediaController: public IEncodeCallback, public IVideoDecodeCallback,
        public std::enable_shared_from_this<MediaController>{
private:
    std::shared_ptr<VideoDecoder> videoDecoder = nullptr;
    std::shared_ptr<VideoDecoder> audioDecoder = nullptr;
    std::shared_ptr<VideoEncoder> videoEncoder = nullptr;
    bool singleDecoder = true;//for one inputFile, else for two inputFile
    bool videoDecodeEnd = false;
    bool audioDecodeEnd = false;
    int encodeDurationMs = DEFAULT_VIDEO_DURATION;
    int encodeFps = DEFAULT_VIDEO_FPS;
    int repeatVideoFrameNum = 1;
    std::string videoOutputPath;
    std::shared_ptr<IVideoDecodeCallback> mediaDecodeCallback = nullptr;
    std::shared_ptr<IEncodeCallback> outsideEncodeCallback = nullptr;
    void onDecodeFrameData(DecodeFrameData data) override;
    void onDecodeMetaData(DecodeMetaData data) override;
    void onEncodeStart() override;
    void onEncodeFinish(int ret, const std::string &encodeFile) override;
public:
    MediaController();
    ~MediaController() override;
    void setEncodeCallback(std::shared_ptr<IEncodeCallback> callback);
    int encodeImgToVideo(const std::string &imgInputPath, const std::string &videoOutputPath);
    int encodeImgAndAudioToVideo(const std::string &imgInputPath, const std::string &audioInputPath,
                                 const std::string &videoOutputPath);
    int encodeVideoToVideo(const std::string &videoInputPath, const std::string &videoOutputPath);
};

#endif //FFMPEGDEMO_MEDIACONTROLLER_H
