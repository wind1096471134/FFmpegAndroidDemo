//
// Created by allan on 2024/11/7.
//

#include "VideoEncoder.h"
#include "Util.h"
#include "map"
#include "Error.h"
extern "C" {
#include "libswscale/swscale.h"
}

#define LOG_TAG "VideoEncoder"

VideoEncoder::VideoEncoder() {
}

VideoEncoder::~VideoEncoder() {
    freeResource();
}

int VideoEncoder::encodeStart(const std::string &outputFile, VideoEncodeParam &param) {
    //init outputFormat
    int ret = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFile.data());
    log(LOG_TAG, "initOutput", ret);
    if(ret < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }

    //init encoder
    codec = avcodec_find_encoder(outputFormatContext->oformat->video_codec);
    if(codec == nullptr) {
        log(LOG_TAG, "find no codec", outputFormatContext->oformat->video_codec);
        freeResource();
        return FFMPEG_API_FAIL;
    }
    avCodecContext = avcodec_alloc_context3(codec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "alloc avCodecContext fail");
        freeResource();
        return FFMPEG_API_FAIL;
    }
    avCodecContext->profile = FF_PROFILE_H264_HIGH;
    avCodecContext->bit_rate = param.bitRate;
    avCodecContext->width = param.w;
    avCodecContext->height = param.h;
    avCodecContext->gop_size = 10;
    avCodecContext->time_base = AVRational {1, param.fps};
    avCodecContext->framerate = AVRational {param.fps, 1};
    avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    ret = avcodec_open2(avCodecContext, codec, nullptr);
    if(ret != 0) {
        log(LOG_TAG, "codec open fail", ret);
        freeResource();
        return FFMPEG_API_FAIL;
    }

    //init output stream
    avStream = avformat_new_stream(outputFormatContext, codec);
    if(avStream == nullptr) {
        log(LOG_TAG, "avformat_new_stream fail");
        freeResource();
        return FFMPEG_API_FAIL;
    }
    avStream->time_base = avCodecContext->time_base;
    if((ret = avio_open(&outputFormatContext->pb, outputFile.data(), AVIO_FLAG_WRITE)) < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }
    if((ret = avcodec_parameters_from_context(avStream->codecpar, avCodecContext)) < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }
    log(LOG_TAG, "write header");
    //write header
    if(avformat_write_header(outputFormatContext, nullptr) < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }

    frameCount = 0;
    return SUC;
}

int VideoEncoder::encodeFrame(const AVFrame *avFrame) {
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
    int ret = sws_scale_frame(swsContext, yuvFrame, avFrame);
    //log(LOG_TAG, "sws_scale_frame", ret, avFrame->format);
    if(ret < 0) {
        log(LOG_TAG, "sws_scale_frame fail", ret, avFrame->format);
        av_frame_free(&yuvFrame);
        return FFMPEG_API_FAIL;
    }
    //set dts and pts
    yuvFrame->pkt_dts = frameCount;
    yuvFrame->pts = yuvFrame->pkt_dts;

    ret = avcodec_send_frame(avCodecContext, yuvFrame);
    if(ret < 0) {
        log(LOG_TAG, "write frame fail ", ret);
        return -1;
    }
    while(ret >= 0) {
        AVPacket *avPacket = av_packet_alloc();
        ret = avcodec_receive_packet(avCodecContext, avPacket);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        avPacket->stream_index = avStream->index;
        //transform pts
        av_packet_rescale_ts(avPacket, avCodecContext->time_base, avStream->time_base);
        if(av_interleaved_write_frame(outputFormatContext, avPacket) != 0) {
            log(LOG_TAG, "av_interleaved_write_frame fail", ret);
        }
        av_packet_unref(avPacket);
    }
    av_frame_free(&yuvFrame);
    frameCount++;

    return SUC;
}

int VideoEncoder::encodeEnd() {
    //write tail
    int ret = av_write_trailer(outputFormatContext);
    freeResource();
    return ret;
}

void VideoEncoder::freeResource() {
    //free res
    codec = nullptr;
    if(avCodecContext != nullptr) {
        avcodec_free_context(&avCodecContext);
    }
    if(outputFormatContext != nullptr) {
        if(outputFormatContext->pb != nullptr) {
            avio_close(outputFormatContext->pb);
        }
        avformat_free_context(outputFormatContext);
        outputFormatContext = nullptr;
    }
    avStream = nullptr;
    frameCount = 0;
}

int videoEncoder_encodeImgToVideo(const AVFrame *avFrame, const std::string &outputFile, VideoEncodeParam &param, int durationSecond) {
    VideoEncoder videoEncoder;
    int ret = videoEncoder.encodeStart(outputFile, param);
    if(ret >= 0) {
        int frameCount = durationSecond * param.fps;
        for(int i = 0; i < frameCount; i++) { //写入每一帧数据
            ret = videoEncoder.encodeFrame(avFrame);
        }
        if(ret >= 0) {
            ret = videoEncoder.encodeEnd();
        }
    }
    return ret;
}
