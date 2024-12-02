//
// Created by allan on 2024/11/24.
//

#include "stdio.h"
#include "Util.h"
#include "thread"
#include "Error.h"
#include "VideoDecoder.h"

#include <utility>

#define LOG_TAG "VideoDecoder"

VideoDecoder::~VideoDecoder() {
    freeResource();
}

void VideoDecoder::freeResource() {
    if(videoCodecContext != nullptr) {
        avcodec_free_context(&videoCodecContext);
    }
    if(audioCodecContext != nullptr) {
        avcodec_free_context(&audioCodecContext);
    }
    if(avFormatContext != nullptr) {
        //avformat_free_context(avFormatContext);
        avformat_close_input(&avFormatContext);
    }
    videoCodec = nullptr;
    audioCodec = nullptr;
    videoStreamId = -1;
    audioStreamId = -1;
}

int VideoDecoder::initCodec(AVMediaType mediaType, AVCodecContext *&avCodecContext,
                            const AVCodec *&avCodec, int &streamId) {
    auto clearRes = [&](){
        if(avCodecContext != nullptr) {
            avcodec_free_context(&avCodecContext);
            avCodecContext = nullptr;
        }
        avCodec = nullptr;
        streamId = -1;
    };
    //find stream
    streamId = av_find_best_stream(avFormatContext, mediaType, -1, -1, nullptr, 0);
    int ret;
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
        log(LOG_TAG, "find stream fail", mediaType);
        clearRes();
        return FFMPEG_API_FAIL;
    }
    //find decoder
    avCodec = avcodec_find_decoder(avFormatContext->streams[streamId]->codecpar->codec_id);
    if(avCodec == nullptr) {
        log(LOG_TAG, "avcodec_find_decoder fail");
        clearRes();
        return FFMPEG_API_FAIL;
    }

    avCodecContext = avcodec_alloc_context3(avCodec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "avcodec_alloc_context3");
        clearRes();
        return FFMPEG_API_FAIL;
    }
    if(avcodec_parameters_to_context(avCodecContext, avFormatContext->streams[streamId]->codecpar) < 0) {
        log(LOG_TAG, "avcodec_parameters_from_context fail", ret);
        clearRes();
        return FFMPEG_API_FAIL;
    }
    //open videoCodec
    ret = avcodec_open2(avCodecContext, avCodec, nullptr);
    if(ret < 0){
        clearRes();
        log(LOG_TAG, "avcodec_open2", ret);
        return FFMPEG_API_FAIL;
    }
    return SUC;
}

int VideoDecoder::decodeFile(const std::string &inputFilePath, DecodeVideoCallback &decodeCallback) {
    freeResource();
    decoding = true;
    std::thread thread([&, inputFilePath](){
        log(LOG_TAG, "decodeFile start ", inputFilePath.data());
        //open input
        int ret = avformat_open_input(&avFormatContext, inputFilePath.data(), nullptr, nullptr);
        if(ret < 0){
            log(LOG_TAG, "avformat_open_input fail", ret);
            freeResource();
            return FFMPEG_API_FAIL;
        }
        //init videoCodec
        int videoRet = initCodec(AVMEDIA_TYPE_VIDEO, videoCodecContext, videoCodec, videoStreamId);
        //init audioCodec
        int audioRet = initCodec(AVMEDIA_TYPE_AUDIO, audioCodecContext, audioCodec, audioStreamId);
        if(videoRet != SUC && audioRet != SUC) {
            freeResource();
            return FFMPEG_API_FAIL;
        }

        //start decode
        AVPacket *avPacket = av_packet_alloc();
        AVFrame *videoFrame = av_frame_alloc();
        AVFrame *audioFrame = av_frame_alloc();

        int frameCount = 0;
        while(decoding) {
            av_packet_unref(avPacket);
            ret = av_read_frame(avFormatContext, avPacket);
            if(ret == 0) {
                AVCodecContext *avCodecContext = nullptr;
                AVMediaType mediaType;
                AVFrame *avFrame;
                if(avPacket->stream_index == videoStreamId) {
                    avCodecContext = videoCodecContext;
                    mediaType = AVMEDIA_TYPE_VIDEO;
                    avFrame = videoFrame;
                } else if (avPacket->stream_index == audioStreamId) {
                    avCodecContext = audioCodecContext;
                    mediaType = AVMEDIA_TYPE_AUDIO;
                    avFrame = audioFrame;
                }
                if(avCodecContext != nullptr) {
                    ret = avcodec_send_packet(avCodecContext, avPacket);
                    if(ret == 0) {
                        ret = avcodec_receive_frame(avCodecContext, avFrame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            continue;
                        } else if (ret < 0) {
                            log(LOG_TAG, "avcodec_receive_frame fail", ret);
                            break;
                        }
                        DecodeFrameData frameData = {frameCount, mediaType, avFrame, false};
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
        av_frame_free(&videoFrame);
        av_frame_free(&audioFrame);
        if(videoStreamId >= 0) {
            decodeCallback({-1, AVMEDIA_TYPE_VIDEO, nullptr, true});
        }
        if(audioStreamId >= 0) {
            decodeCallback({-1, AVMEDIA_TYPE_AUDIO, nullptr, true});
        }
        freeResource();

        log(LOG_TAG, "decodeFile end", inputFilePath.data());

        return SUC;
    });
    thread.detach();
    return SUC;
}

void VideoDecoder::stopDecode() {
    decoding = false;
}

VideoDecoder::VideoDecoder() {

}
