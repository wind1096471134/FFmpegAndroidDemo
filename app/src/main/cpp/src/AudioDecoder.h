//
// Created by allan on 2024/11/24.
//

#ifndef FFMPEGDEMO_AUDIODECODER_H
#define FFMPEGDEMO_AUDIODECODER_H

#include "BaseDecoder.h"

class AudioDecoder: public BaseDecoder {
public:
    AudioDecoder(): BaseDecoder(AVMEDIA_TYPE_AUDIO) {};
    ~AudioDecoder() {};
};

#endif //FFMPEGDEMO_AUDIODECODER_H