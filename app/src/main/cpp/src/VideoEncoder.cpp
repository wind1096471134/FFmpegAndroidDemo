//
// Created by 10964 on 2024/11/7.
//

#include "VideoEncoder.h"
#include "Util.h"

#define LOG_TAG "VideoEncoder"
#define IMG_WH 512
#define FPS 25

int VideoEncoder::encodeImgToVideo(const AVFrame *imgFrame, const char *outputFilePath) {
    //init output
    int ret = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilePath);
    log(LOG_TAG, "initOutput", ret);
    if(ret != 0) {
        destroy();
        return ret;
    }

    //init input
    int imgW = imgFrame->width;
    int imgH = imgFrame->height;
    if(imgFrame->format != AV_PIX_FMT_YUV420P) {
        log(LOG_TAG, "input format wrong", imgFrame->format);
        destroy();
        return -1;
    }

    //init encoder
    codec = avcodec_find_encoder(outputFormatContext->oformat->video_codec);
    if(codec == nullptr) {
        log(LOG_TAG, "find no codec", outputFormatContext->oformat->video_codec);
        destroy();
        return -1;
    }
    avCodecContext = avcodec_alloc_context3(codec);
    if(avCodecContext == nullptr) {
        log(LOG_TAG, "alloc avCodecContext fail");
        destroy();
        return -1;
    }
    avCodecContext->profile = FF_PROFILE_H264_HIGH;
    avCodecContext->bit_rate = 3000000;
    avCodecContext->width = imgW;
    avCodecContext->height = imgH;
    avCodecContext->gop_size = 10;
    avCodecContext->time_base = AVRational {1, FPS};
    avCodecContext->framerate = AVRational {FPS, 1};
    avCodecContext->pix_fmt = static_cast<AVPixelFormat>(imgFrame->format);
    ret = avcodec_open2(avCodecContext, codec, nullptr);
    if(ret != 0) {
        log(LOG_TAG, "codec open fail", ret);
        destroy();
        return ret;
    }

    //init output stream
    AVStream *avStream = avformat_new_stream(outputFormatContext, codec);
    avStream->time_base = avCodecContext->time_base;
    if(avcodec_parameters_from_context(avStream->codecpar, avCodecContext) >= 0) {
        if(avio_open(&outputFormatContext->pb, outputFilePath, AVIO_FLAG_WRITE) >= 0) {
            log(LOG_TAG, "write header");
            //write header
            if(avformat_write_header(outputFormatContext, nullptr) >= 0) {
                log(LOG_TAG, "write frame");
                //write frame
                int frameCount = 0;
                AVPacket *avPacket = av_packet_alloc();
                av_init_packet(avPacket);
                avPacket->data = nullptr;
                avPacket->size = 0;

                AVFrame *avFrame = av_frame_alloc();
                avFrame->width = avCodecContext->width;
                avFrame->height = avCodecContext->height;
                avFrame->format = avCodecContext->pix_fmt;
                av_frame_get_buffer(avFrame, 0);

                while(frameCount < 150) {
                    //write data and pts/dts
                    avFrame->data[0] = imgFrame->data[0];
                    avFrame->data[1] = imgFrame->data[1];
                    avFrame->data[2] = imgFrame->data[2];
                    avFrame->pkt_dts = frameCount;
                    avFrame->pts = avFrame->pkt_dts;

                    //log("frame", avFrame->pts);

                    int ret = avcodec_send_frame(avCodecContext, avFrame);
                    if(ret < 0) {
                        log(LOG_TAG, "write frame fail ", ret);
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
                    frameCount++;
                }
                av_frame_free(&avFrame);
            }
            //write tail
            int ret = av_write_trailer(outputFormatContext);
            log(LOG_TAG, "write tail", ret);
        } else {
            log(LOG_TAG, "io_open fail");
        }
    } else {
        log(LOG_TAG, "avcodec_parameters_from_context fail");
    }
    destroy();

    return 0;
}

void VideoEncoder::destroy() {
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
}

VideoEncoder::VideoEncoder() {
}
