//
// Created by allan on 2024/11/24.
//
extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
}
#include "stdio.h"
#include "Util.h"
#include "BaseDecoder.h"

#define LOG_TAG "Decoder"

BaseDecoder::~BaseDecoder() {
    freeResource();
}

void BaseDecoder::freeResource() {
    if(avCodecContext != nullptr) {
        avcodec_free_context(&avCodecContext);
    }
    if(avFormatContext != nullptr) {
        //avformat_free_context(avFormatContext);
        avformat_close_input(&avFormatContext);
    }
    avCodec = nullptr;
}

int BaseDecoder::decodeFile(const char *filePath, DecodeVideoCallback &decodeCallback) {
    //open input
    int ret = avformat_open_input(&avFormatContext, filePath, nullptr, nullptr);
    if(ret < 0){
        log(LOG_TAG, "avformat_open_input fail", ret);
        freeResource();
        return -1;
    }
    //find stream
    int streamId = av_find_best_stream(avFormatContext, mediaType, -1, -1, nullptr, 0);
    if(streamId < 0) {
        //try another way
        ret = avformat_find_stream_info(avFormatContext, nullptr);
        if(ret >= 0) {
            for(int i = 0; i < avFormatContext->nb_streams; i++) {
                AVStream *stream = avFormatContext->streams[i];
                if(stream->codecpar->codec_type == mediaType) {
                    streamId = i;
                    break;
                }
            }
        }
    }
    if(streamId < 0) {
        log(LOG_TAG, "find stream fail", streamId);
        freeResource();
        return -1;
    }
    //find decoder
    avCodec = avcodec_find_decoder(avFormatContext->streams[streamId]->codecpar->codec_id);
    if(avCodec == nullptr) {
        log(LOG_TAG, "avcodec_find_decoder fail");
        freeResource();
        return -1;
    }

    avCodecContext = avcodec_alloc_context3(avCodec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "avcodec_alloc_context3");
        freeResource();
        return -1;
    }
    if(avcodec_parameters_to_context(avCodecContext, avFormatContext->streams[streamId]->codecpar) < 0) {
        log(LOG_TAG, "avcodec_parameters_from_context fail", ret);
        freeResource();
        return -1;
    }
    //open codec
    ret = avcodec_open2(avCodecContext, avCodec, nullptr);
    if(ret < 0){
        log(LOG_TAG, "avcodec_open2", ret);
        freeResource();
        return -1;
    }

    //start decode
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = avCodecContext->width;
    avFrame->height = avCodecContext->height;
    avFrame->format = avCodecContext->pix_fmt;
    av_frame_get_buffer(avFrame, 0);

    int frameCount = 0;
    while(true) {
        av_packet_unref(avPacket);
        ret = av_read_frame(avFormatContext, avPacket);
        if(ret == 0) {
            if(avPacket->stream_index == streamId) {
                ret = avcodec_send_packet(avCodecContext, avPacket);
                if(ret == 0) {
                    ret = avcodec_receive_frame(avCodecContext, avFrame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        continue;
                    } else if (ret < 0) {
                        log(LOG_TAG, "avcodec_receive_frame fail", ret);
                        break;
                    }

                    DecodeFrameData frameData = {frameCount, mediaType, avFrame};
                    decodeCallback(frameData);
                    frameCount++;

                } else {
                    log(LOG_TAG, "avcodec_send_packet fail", ret);
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
    freeResource();
    return 0;
}
