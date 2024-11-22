//
// Created by allan on 2024/11/14.
//

extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
}
#include "VideoDecoder.h"
#include "stdio.h"
#include "Util.h"

#define LOG_TAG "VideoDecoder"

int VideoDecoder::decodeFile(const char *filePath, DecodeVideoCallback &decodeCallback) {
    return decodeVideo(filePath, decodeCallback);
}

void VideoDecoder::destroy() {
    if(avCodecContext != nullptr) {
        avcodec_free_context(&avCodecContext);
    }
    if(avFormatContext != nullptr) {
        //avformat_free_context(avFormatContext);
        avformat_close_input(&avFormatContext);
    }
    avCodec = nullptr;
}

VideoDecoder::VideoDecoder() {

}

int VideoDecoder::decodeVideo(const char *filePath, DecodeVideoCallback &decodeCallback) {
    int ret = avformat_open_input(&avFormatContext, filePath, nullptr, nullptr);
    if(ret < 0){
        log(LOG_TAG, "avformat_open_input fail", ret);
        destroy();
        return -1;
    }
    int streamId = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &avCodec, 0);
    if(streamId < 0 || avCodec == nullptr) {
        log(LOG_TAG, "av_find_best_stream fail", ret);
        destroy();
        return -1;
    }
    avCodecContext = avcodec_alloc_context3(avCodec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "avcodec_alloc_context3");
        destroy();
        return -1;
    }
    if(avcodec_parameters_to_context(avCodecContext, avFormatContext->streams[streamId]->codecpar) < 0) {
        log(LOG_TAG, "avcodec_parameters_from_context fail", ret);
        destroy();
        return -1;
    }
    ret = avcodec_open2(avCodecContext, avCodec, nullptr);
    if(ret < 0){
        log(LOG_TAG, "avcodec_open2", ret);
        destroy();
        return -1;
    }

    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = avCodecContext->width;
    avFrame->height = avCodecContext->height;
    avFrame->format = avCodecContext->pix_fmt;
    av_frame_get_buffer(avFrame, 0);

    while(true) {
        av_packet_unref(avPacket);
        ret = av_read_frame(avFormatContext, avPacket);
        if(ret == 0) {
            if(avPacket->stream_index == streamId) {
                ret = avcodec_send_packet(avCodecContext, avPacket);
                log(LOG_TAG, "avcodec_send_packet", ret);
                if(ret == 0) {
                    ret = avcodec_receive_frame(avCodecContext, avFrame);
                    log(LOG_TAG, "avcodec_receive_frame", ret);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        continue;
                    } else if (ret < 0) {
                        break;
                    }
                    //transform data to yuv420p
                    SwsContext *swsContext = sws_getContext(
                            avFrame->width, avFrame->height, static_cast<AVPixelFormat>(avFrame->format),
                            avFrame->width, avFrame->height, AV_PIX_FMT_YUV420P,
                            0, nullptr, nullptr, nullptr);
                    AVFrame *yuvFrame = av_frame_alloc();
                    yuvFrame->width = avFrame->width;
                    yuvFrame->height = avFrame->height;
                    yuvFrame->format = AV_PIX_FMT_YUV420P;
                    av_frame_get_buffer(yuvFrame, 0);
                    ret = sws_scale_frame(swsContext, yuvFrame, avFrame);
                    log(LOG_TAG, "sws_scale_frame", ret, avFrame->format);

                    if(ret >= 0) {
                        //callback
                        DecodeFrameData frameData = {0, 1, yuvFrame};
                        decodeCallback(frameData);
                    }
                    av_frame_free(&yuvFrame);
                } else {
                    break;
                }
            }
        } else {
            log(LOG_TAG, "read_frame fail", ret);
            break;
        }
    }
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);
    destroy();
    return 0;
}