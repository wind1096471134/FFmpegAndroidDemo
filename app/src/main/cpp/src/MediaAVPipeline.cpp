//
// Created by allan on 2024/12/22.
//

#include "MediaAVPipeline.h"

void MediaAVPipeline::processFrame(AVFrame *avFrame) {
    AVFrame *in = avFrame;
    while(!processNodes->empty()) {
        std::shared_ptr<AVProcessNode> processNode = processNodes->front();
        in = processNode->processFrame(in);
    }
    sink->processFrame(in);
}

void MediaAVPipeline::release() {
    while(!processNodes->empty()) {
        processNodes->front()->release();
        processNodes->pop();
    }
    sink->release();
}

MediaAVPipeline::MediaAVPipeline(std::shared_ptr<AVProcessSink> sink): sink(sink), nodeMutex() {
    processNodes = std::make_shared<std::queue<std::shared_ptr<AVProcessNode>>>();
}

int MediaAVPipeline::addProcessNode(std::shared_ptr<AVProcessNode> node) {
    std::lock_guard<std::mutex> lockGuard(nodeMutex);
    processNodes->push(node);
    return 1;
}
