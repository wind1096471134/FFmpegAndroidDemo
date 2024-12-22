//
// Created by allan on 2024/12/22.
//

#ifndef FFMPEGDEMO_MEDIAAVPIPELINE_H
#define FFMPEGDEMO_MEDIAAVPIPELINE_H

#include "AVProcessNode.h"
#include "AVProcessSink.h"
#include "queue"
#include "mutex"

class MediaAVPipeline: public AVProcessSink {
private:
    std::mutex nodeMutex;
    std::shared_ptr<std::queue<std::shared_ptr<AVProcessNode>>> processNodes;
    std::shared_ptr<AVProcessSink> sink;
public:
    MediaAVPipeline(std::shared_ptr<AVProcessSink> sink);
    int addProcessNode(std::shared_ptr<AVProcessNode> node);
    void processFrame(AVFrame *avFrame) override;
    void release() override;
};
#endif //FFMPEGDEMO_MEDIAAVPIPELINE_H
