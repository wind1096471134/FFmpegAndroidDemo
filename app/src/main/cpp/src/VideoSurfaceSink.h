//
// Created by allan on 2024/12/14.
//

#ifndef FFMPEGDEMO_VIDEOSURFACESINK_H
#define FFMPEGDEMO_VIDEOSURFACESINK_H

#include <android/native_window_jni.h>
#include "AVSink.h"
#include "mutex"

class VideoSurfaceSink: public AVSink {
private:
    std::mutex mutex;
    ANativeWindow *nativeWindow;
    void freeRes();
public:
    explicit VideoSurfaceSink(ANativeWindow *nativeWindow);
    ~VideoSurfaceSink();
    void setSurfaceSize(int width, int height);
    void processFrame(AVFrame *avFrame) override;
    void release() override;
};

#endif //FFMPEGDEMO_VIDEOSURFACESINK_H
