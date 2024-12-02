//
// Created by allan on 2024/11/28.
//

#ifndef FFMPEGDEMO_BLOCKINGQUEUE_H
#define FFMPEGDEMO_BLOCKINGQUEUE_H

#include "queue"
#include "mutex"

template<typename T>
class BlockingQueue {
private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable notEmpty;
public:
    BlockingQueue() = default;
    void enqueue(const T &item);
    T dequeue();
    bool isEmpty();
};

template<typename T>
void BlockingQueue<T>::enqueue(const T &item) {
    std::lock_guard<std::mutex> lockGuard(mutex);
    queue.push(item);
    notEmpty.notify_all();
}

template<typename T>
T BlockingQueue<T>::dequeue() {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]{ return !queue.empty();});
    T item = queue.front();
    queue.pop();
    return item;
}

template<typename T>
bool BlockingQueue<T>::isEmpty() {
    std::lock_guard<std::mutex> lockGuard(mutex);
    return queue.empty();
}

#endif //FFMPEGDEMO_BLOCKINGQUEUE_H
