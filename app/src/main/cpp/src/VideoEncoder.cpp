//
// Created by allan on 2024/11/7.
//

#include "VideoEncoder.h"
#include "Util.h"
#include "map"
#include "Error.h"
#include "thread"
extern "C" {
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

#define LOG_TAG "VideoEncoder"

VideoEncoder::VideoEncoder(): queue(), encodeRunning(false), audioFrameInputSampleNum(0),
                              videoFrameInputNum(0), audioFramePts(0), videoFramePts(0),
                              videoEncodeParam(), audioEncodeParam() {
}

VideoEncoder::~VideoEncoder() {
    freeResource();
}

int VideoEncoder::encodeStart(const std::string &outputFile, const VideoEncodeParam &videoEncodeParam, const AudioEncodeParam &audioEncodeParam) {
    encodeRunning.store(true);
    this->videoEncodeParam = videoEncodeParam;
    this->audioEncodeParam = audioEncodeParam;
    std::thread thread([&, outputFile]() {
        if(encodeCallback != nullptr) {
            encodeCallback->onEncodeStart();
        }

        log(LOG_TAG, "encodeStart", outputFile.data());
        int ret = encodeThreadHandlerLoop(outputFile, this->videoEncodeParam, this->audioEncodeParam);
        log(LOG_TAG, "encodeEnd", ret);

        if(encodeCallback != nullptr) {
            encodeCallback->onEncodeFinish(ret, outputFile);
        }
        freeResource();
    });
    thread.detach();
    return SUC;
}

int VideoEncoder::encodeThreadHandlerLoop(const std::string &outputFile,
                                          const VideoEncodeParam &videoEncodeParam,
                                          const AudioEncodeParam &audioEncodeParam) {
    //init outputFormat
    int ret = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFile.data());
    log(LOG_TAG, "initOutput", ret);
    if(ret < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }

    //------- init videoCodec start ------
    if(videoEncodeParam.encode) {
        videoCodec = avcodec_find_encoder(outputFormatContext->oformat->video_codec);
        if(videoCodec == nullptr) {
            log(LOG_TAG, "find no videoCodec", outputFormatContext->oformat->video_codec);
            freeResource();
            return FFMPEG_API_FAIL;
        }
        videoCodecContext = avcodec_alloc_context3(videoCodec);
        if(videoCodecContext == nullptr) {
            log(LOG_TAG, "alloc videoCodecContext fail");
            freeResource();
            return FFMPEG_API_FAIL;
        }
        videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
        videoCodecContext->bit_rate = videoEncodeParam.bitRate;
        videoCodecContext->width = videoEncodeParam.w;
        videoCodecContext->height = videoEncodeParam.h;
        videoCodecContext->gop_size = 10;
        videoCodecContext->time_base = AVRational {1, videoEncodeParam.fps};
        videoCodecContext->framerate = AVRational {videoEncodeParam.fps, 1};
        videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
        ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
        if(ret != 0) {
            log(LOG_TAG, "videoCodec open fail", ret);
            freeResource();
            return FFMPEG_API_FAIL;
        }

        //init output stream
        videoStream = avformat_new_stream(outputFormatContext, videoCodec);
        if(videoStream == nullptr) {
            log(LOG_TAG, "avformat_new_stream fail");
            freeResource();
            return FFMPEG_API_FAIL;
        }
        videoStream->time_base = videoCodecContext->time_base;
        if((ret = avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext)) < 0) {
            freeResource();
            return FFMPEG_API_FAIL;
        }
    }
    //------- init videoCodec end ------

    //------- init audioCodec start ------
    if(audioEncodeParam.encode) {
        audioCodec = avcodec_find_encoder(outputFormatContext->oformat->audio_codec);
        if(audioCodec == nullptr) {
            log(LOG_TAG, "find no audioCodec", outputFormatContext->oformat->audio_codec);
            freeResource();
            return FFMPEG_API_FAIL;
        }
        audioCodecContext = avcodec_alloc_context3(audioCodec);
        if(audioCodecContext == nullptr) {
            log(LOG_TAG, "alloc audioCodecContext fail");
            freeResource();
            return FFMPEG_API_FAIL;
        }
        audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
        audioCodecContext->sample_rate = audioEncodeParam.sampleRate;
        audioCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
        audioCodecContext->bit_rate = audioEncodeParam.bitRate;
        audioCodecContext->ch_layout = audioEncodeParam.channelLayout;
        audioCodecContext->time_base = AVRational {1, audioEncodeParam.sampleRate};
        ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
        if(ret != 0) {
            log(LOG_TAG, "videoCodec open fail", ret);
            freeResource();
            return FFMPEG_API_FAIL;
        }

        //init output stream
        audioStream = avformat_new_stream(outputFormatContext, audioCodec);
        if(audioStream == nullptr) {
            log(LOG_TAG, "avformat_new_stream fail");
            freeResource();
            return FFMPEG_API_FAIL;
        }
        audioStream->time_base = audioCodecContext->time_base;
        if((ret = avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext)) < 0) {
            freeResource();
            return FFMPEG_API_FAIL;
        }
    }
    //------- init audioCodec end ------

    //write header
    log(LOG_TAG, "write header");
    if((ret = avio_open(&outputFormatContext->pb, outputFile.data(), AVIO_FLAG_WRITE)) < 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }
    if((ret = avformat_write_header(outputFormatContext, nullptr)) < 0) {
        log(LOG_TAG, "write header fail", ret);
        freeResource();
        return FFMPEG_API_FAIL;
    }

    //loop and encode frames.
    while (encodeRunning || !queue.isEmpty()) {
        EncodeFrame *encodeFrame = nullptr;
        if(!queue.dequeue(&encodeFrame) || encodeFrame == nullptr) {
            continue;
        }

        ret = encodeFrameInternal(*encodeFrame);
        av_frame_free(&(*encodeFrame).avFrame);
        if(ret != SUC) {
            freeResource();
            return ret;
        }
    }
    //write tail and finish.
    ret = av_write_trailer(outputFormatContext);
    if(ret != 0) {
        freeResource();
        return FFMPEG_API_FAIL;
    }
    freeResource();
    return SUC;
}

int VideoEncoder::encodeFrame(const EncodeFrame &encodeFrame) {
    if(!encodeRunning.load()) {
        return ENCODE_ALREADY_FINISH;
    }
    AVFrame *copyFrame = av_frame_clone(encodeFrame.avFrame);
    if(copyFrame == nullptr) {
        return FFMPEG_API_FAIL;
    }
    queue.enqueue({copyFrame, encodeFrame.mediaType});
    if(encodeFrame.mediaType == AVMEDIA_TYPE_VIDEO) {
        videoFrameInputNum++;
    } else if (encodeFrame.mediaType == AVMEDIA_TYPE_AUDIO) {
        audioFrameInputSampleNum += copyFrame->nb_samples;
    }
    //log(LOG_TAG, "encodeFrame", videoFrameInputNum, audioFrameInputSampleNum);

    return SUC;
}

int VideoEncoder::encodeFrameInternal(const EncodeFrame &encodeFrame) {
    AVFrame *avFrame = encodeFrame.avFrame;
    AVFrame *dstFrame = nullptr;
    AVCodecContext *avCodecContext = nullptr;
    AVStream *avStream = nullptr;
    if(encodeFrame.mediaType == AVMEDIA_TYPE_VIDEO) {
        //log(LOG_TAG, "write video frame", videoFramePts);
        //transform data to yuv420p
        SwsContext *swsContext = sws_getContext(
                avFrame->width, avFrame->height, static_cast<AVPixelFormat>(avFrame->format),
                videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt,
                0, nullptr, nullptr, nullptr);
        if(swsContext == nullptr) {
            log(LOG_TAG, "sws_getContext fail");
            return FFMPEG_API_FAIL;
        }
        AVFrame *yuvFrame = av_frame_alloc();
        yuvFrame->width = videoCodecContext->width;
        yuvFrame->height = videoCodecContext->height;
        yuvFrame->format = videoCodecContext->pix_fmt;
        yuvFrame->time_base = videoCodecContext->time_base;
        av_frame_get_buffer(yuvFrame, 0);
        int ret = sws_scale_frame(swsContext, yuvFrame, avFrame);
        sws_freeContext(swsContext);
        //log(LOG_TAG, "sws_scale_frame", ret, avFrame->format);
        if(ret < 0) {
            log(LOG_TAG, "sws_scale_frame fail", ret, avFrame->format);
            av_frame_free(&yuvFrame);
            return FFMPEG_API_FAIL;
        }
        yuvFrame->pkt_dts = videoFramePts;
        yuvFrame->pts = yuvFrame->pkt_dts;
        videoFramePts++;

        dstFrame = yuvFrame;
        avCodecContext = videoCodecContext;
        avStream = videoStream;
    } else if (encodeFrame.mediaType == AVMEDIA_TYPE_AUDIO) {
        //log(LOG_TAG, "write audio frame", audioFramePts, getEncodeAudioDuration());
        SwrContext *swrContext = nullptr;
        int ret = swr_alloc_set_opts2(&swrContext, &audioCodecContext->ch_layout, audioCodecContext->sample_fmt, audioCodecContext->sample_rate,
                                      &avFrame->ch_layout, static_cast<AVSampleFormat>(avFrame->format), avFrame->sample_rate, 0,
                                      nullptr);
        if(ret < 0 || swr_init(swrContext) < 0) {
            log(LOG_TAG, "swr_alloc_set_opts2 fail", ret);
            return FFMPEG_API_FAIL;
        }
        AVFrame *audioFrame = av_frame_alloc();
        audioFrame->ch_layout = audioCodecContext->ch_layout;
        audioFrame->format = audioCodecContext->sample_fmt;
        audioFrame->sample_rate = audioCodecContext->sample_rate;
        audioFrame->time_base = audioCodecContext->time_base;

        av_frame_get_buffer(audioFrame, 0);
        ret = swr_convert_frame(swrContext, audioFrame, avFrame);
        swr_free(&swrContext);
        if(ret < 0) {
            char *errMsg = new char[20];
            av_strerror(ret, errMsg, 20);
            log(LOG_TAG, "swr_convert_frame fail", errMsg);
            delete []errMsg;
            av_frame_free(&audioFrame);
            return FFMPEG_API_FAIL;
        }
        audioFrame->pts = audioFramePts;
        audioFrame->pkt_dts = audioFramePts;
        audioFramePts += audioFrame->nb_samples;

        dstFrame = audioFrame;
        avCodecContext = audioCodecContext;
        avStream = audioStream;
    }

    if(dstFrame == nullptr || avCodecContext == nullptr || avStream == nullptr) {
        return FFMPEG_API_FAIL;
    }

    int ret = avcodec_send_frame(avCodecContext, dstFrame);
    if(ret < 0) {
        log(LOG_TAG, "write frame fail ", ret);
        return FFMPEG_API_FAIL;
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
        int ret1 = av_interleaved_write_frame(outputFormatContext, avPacket);
        av_packet_free(&avPacket);
        if(ret1 != 0) {
            log(LOG_TAG, "av_interleaved_write_frame fail", ret1);
            return FFMPEG_API_FAIL;
        }
    }
    av_frame_free(&dstFrame);

    return SUC;
}

int VideoEncoder::encodeEnd() {
    log(LOG_TAG, "call encodeEnd");
    encodeRunning.store(false);
    return SUC;
}

void VideoEncoder::freeResource() {
    //free res
    videoCodec = nullptr;
    if(videoCodecContext != nullptr) {
        avcodec_free_context(&videoCodecContext);
    }
    audioCodec = nullptr;
    if(audioCodecContext != nullptr) {
        avcodec_free_context(&audioCodecContext);
    }
    if(outputFormatContext != nullptr) {
        if(outputFormatContext->pb != nullptr) {
            avio_close(outputFormatContext->pb);
        }
        avformat_free_context(outputFormatContext);
        outputFormatContext = nullptr;
    }
    queue.shutdown();
    while(!queue.isEmpty()) {
        EncodeFrame *frame = nullptr;
        if(queue.dequeueNonBlock(&frame) && frame != nullptr) {
            av_frame_free(&(*frame).avFrame);
        }
    }
    videoStream = nullptr;
    audioStream = nullptr;
    videoFramePts = 0;
    audioFramePts = 0;
    videoFrameInputNum = 0;
    audioFrameInputSampleNum = 0;
    videoEncodeParam.store({});
    audioEncodeParam.store({});
}

void VideoEncoder::setEncodeCallback(std::shared_ptr<IEncodeCallback> encodeCallback) {
    this->encodeCallback = encodeCallback;
}

int VideoEncoder::getEncodeVideoDuration() {
    int dur = videoFrameInputNum  * 1000 / videoEncodeParam.load().fps;
    return dur;
}

int VideoEncoder::getEncodeAudioDuration() {
    int dur = audioFrameInputSampleNum  * 1000 / audioEncodeParam.load().sampleRate;
    return dur;
}

bool VideoEncoder::isEncoding() {
    return encodeRunning;
}
