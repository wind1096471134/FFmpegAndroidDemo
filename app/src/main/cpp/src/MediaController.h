//
// Created by allan on 2024/11/16.
//

#ifndef FFMPEGDEMO_MEDIACONTROLLER_H
#define FFMPEGDEMO_MEDIACONTROLLER_H

class MediaController{
public:
    MediaController();
    ~MediaController();
    int encodeImgToVideo(const char *imgInputPath, const char *videoOutputPath);
    int encodeImgAndAudioToVideo(const char *imgInputPath, const char *audioInputPath, const char *videoOutputPath);
};
#endif //FFMPEGDEMO_MEDIACONTROLLER_H
