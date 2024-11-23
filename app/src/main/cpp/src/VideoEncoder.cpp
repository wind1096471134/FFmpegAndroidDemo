//
// Created by 10964 on 2024/11/7.
//

#include "VideoEncoder.h"
#include "Util.h"
#include "map"

#define LOG_TAG "VideoEncoder"

VideoEncoder::VideoEncoder() {
}

VideoEncoder::~VideoEncoder() {
    freeResource();
}

int VideoEncoder::encodeStart(const char *outputFile, VideoEncodeParam &param) {
    //init outputFormat
    int ret = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFile);
    log(LOG_TAG, "initOutput", ret);
    if(ret < 0) {
        freeResource();
        return ret;
    }

    //init encoder
    codec = avcodec_find_encoder(outputFormatContext->oformat->video_codec);
    if(codec == nullptr) {
        log(LOG_TAG, "find no codec", outputFormatContext->oformat->video_codec);
        freeResource();
        return -1;
    }
    avCodecContext = avcodec_alloc_context3(codec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "alloc avCodecContext fail");
        freeResource();
        return -1;
    }
    avCodecContext->profile = FF_PROFILE_H264_HIGH;
    avCodecContext->bit_rate = param.bitRate;
    avCodecContext->width = param.w;
    avCodecContext->height = param.h;
    avCodecContext->gop_size = 10;
    avCodecContext->time_base = AVRational {1, param.fps};
    avCodecContext->framerate = AVRational {param.fps, 1};
    avCodecContext->pix_fmt = static_cast<AVPixelFormat>(param.inputDataFormat);
    ret = avcodec_open2(avCodecContext, codec, nullptr);
    if(ret != 0) {
        log(LOG_TAG, "codec open fail", ret);
        freeResource();
        return ret;
    }

    //init output stream
    avStream = avformat_new_stream(outputFormatContext, codec);
    if(avStream == nullptr) {
        log(LOG_TAG, "avformat_new_stream fail");
        freeResource();
        return ret;
    }
    avStream->time_base = avCodecContext->time_base;
    if((ret = avio_open(&outputFormatContext->pb, outputFile, AVIO_FLAG_WRITE)) < 0) {
        freeResource();
        return ret;
    }
    if((ret = avcodec_parameters_from_context(avStream->codecpar, avCodecContext)) < 0) {
        freeResource();
        return ret;
    }
    log(LOG_TAG, "write header");
    //write header
    if(avformat_write_header(outputFormatContext, nullptr) < 0) {
        freeResource();
        return ret;
    }

    frameCount = 0;
    return 0;
}

int VideoEncoder::encodeFrame(const AVFrame *imgFrame) {
    log(LOG_TAG, "write frame");
    //write frame
    AVPacket *avPacket = av_packet_alloc();
    av_init_packet(avPacket);
    avPacket->data = nullptr;
    avPacket->size = 0;

    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = avCodecContext->width;
    avFrame->height = avCodecContext->height;
    avFrame->format = imgFrame->format;
    av_frame_get_buffer(avFrame, 0);

    //write data and pts/dts
    avFrame->data[0] = imgFrame->data[0];
    avFrame->data[1] = imgFrame->data[1];
    avFrame->data[2] = imgFrame->data[2];
    avFrame->pkt_dts = frameCount;
    avFrame->pts = avFrame->pkt_dts;

    int ret = avcodec_send_frame(avCodecContext, avFrame);
    if(ret < 0) {
        log(LOG_TAG, "write frame fail ", ret);
        return -1;
    }
    while(ret >= 0) {
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
    av_frame_free(&avFrame);
    frameCount++;

    return 0;
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

int videoEncoder_encodeImgToVideo(const AVFrame *avFrame, const char *outputFile, VideoEncodeParam &param, int durationSecond) {
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
