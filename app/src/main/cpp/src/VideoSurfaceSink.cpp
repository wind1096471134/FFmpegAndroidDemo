//
// Created by allan on 2024/12/14.
//

#include "VideoSurfaceSink.h"
#include "Util.h"

#define LOG_TAG "VideoSurfaceSink"

VideoSurfaceSink::VideoSurfaceSink(ANativeWindow *nativeWindow) {
    ANativeWindow_acquire(nativeWindow);
    this->nativeWindow = nativeWindow;
}

VideoSurfaceSink::~VideoSurfaceSink() {
    freeRes();
}

void VideoSurfaceSink::processFrame(AVFrame *avFrame) {
    if(nativeWindow != nullptr) {
        ANativeWindow_Buffer buffer;
        int ret = ANativeWindow_lock(nativeWindow, &buffer, nullptr);
        if(ret == 0) {
            uint8_t *dstBuffer = (uint8_t *) buffer.bits;
            int dstStride = buffer.stride * 4;
            int srcStride = avFrame->linesize[0];
            for (int h = 0; h < avFrame->height; h++) {
                memcpy(dstBuffer + h * dstStride, avFrame->data[0] + h * srcStride, srcStride);
            }
            ret = ANativeWindow_unlockAndPost(nativeWindow);
        }
    }
}

void VideoSurfaceSink::release() {
   freeRes();
}

void VideoSurfaceSink::freeRes() {
    if(nativeWindow != nullptr) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = nullptr;
    }
}

void VideoSurfaceSink::setSurfaceSize(int width, int height) {
    int ret = ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    log(LOG_TAG, "ANativeWindow_setBuffersGeometry", ret);
}
