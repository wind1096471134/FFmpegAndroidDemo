//
// Created by allan on 2024/12/14.
//
#include "AudioTrackSink.h"

AudioTrackSink::AudioTrackSink(std::shared_ptr<NativeAudioTrackWrapper> audioTrack) {
    this->audioTrack = audioTrack;
}

AudioTrackSink::~AudioTrackSink() {
    freeRes();
}

void AudioTrackSink::processFrame(AVFrame *avFrame) {
    this->audioTrack->playFrame(avFrame->data[0], avFrame->linesize[0]);
    av_frame_free(&avFrame);
}

void AudioTrackSink::release() {
    freeRes();
}

void AudioTrackSink::setAudioConfig(int sampleRate, AVChannelLayout channelLayout,
                                    AVSampleFormat sampleFormat) {
    int channelConfig;
    const AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    const AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
    if(av_channel_layout_compare(&stereo, &channelLayout) == 0) {
        channelConfig = 2;
    } else if (av_channel_layout_compare(&mono, &channelLayout) == 0) {
        channelConfig = 1;
    } else {
        channelConfig = 2;
    }
    int audioFormat = 2;
    if(sampleFormat == AV_SAMPLE_FMT_S16) {
        audioFormat = 2;
    } else if (sampleFormat == AV_SAMPLE_FMT_U8) {
        audioFormat = 1;
    }
    audioTrack->playStart(sampleRate, channelConfig, audioFormat);
}

void AudioTrackSink::freeRes() {
    this->audioTrack->playEnd();
}

