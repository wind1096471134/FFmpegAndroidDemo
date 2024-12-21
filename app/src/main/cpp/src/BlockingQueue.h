//
// Created by allan on 2024/11/28.
//

#ifndef FFMPEGDEMO_BLOCKINGQUEUE_H
#define FFMPEGDEMO_BLOCKINGQUEUE_H

#include "queue"
#include "mutex"
#include "stdexcept"
#include "Util.h"

template<typename T>
class BlockingQueue {
private:
    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable notEmpty;
    std::condition_variable notFull;
    int capacity;
    std::atomic<bool> isShutdown;
public:
    explicit BlockingQueue(int capacity = INT_MAX): capacity(capacity), isShutdown(false) {};
    ~BlockingQueue();
    void enqueue(const T &item);
    int dequeue(T **out);
    int dequeueNonBlock(T **out);
    bool isEmpty();
    void shutdown();
};

template<typename T>
BlockingQueue<T>::~BlockingQueue() = default;

template<typename T>
void BlockingQueue<T>::shutdown() {
    log("BlockingQueue", "shutdown", capacity);
    isShutdown = true;
    notFull.notify_all();
    notEmpty.notify_all();
}

template<typename T>
void BlockingQueue<T>::enqueue(const T &item) {
    std::unique_lock<std::mutex> lock(mutex);
    notFull.wait(lock, [this] {return isShutdown || queue.size() < capacity;});
    queue.push(item);
    notEmpty.notify_all();
}

template<typename T>
int BlockingQueue<T>::dequeue(T **out) {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]{ return isShutdown || !queue.empty();});
    if(queue.empty() || isShutdown) {
        *out = nullptr;
        return 0;
    }
    T item = queue.front();
    *out = new T(item);
    queue.pop();
    notFull.notify_all();
    return 1;
}

template<typename T>
int BlockingQueue<T>::dequeueNonBlock(T **out) {
    std::unique_lock<std::mutex> lock(mutex);
    if(queue.size() == 0) {
        *out = nullptr;
        return 0;
    }
    T item = queue.front();
    *out = new T(item);
    queue.pop();
    notFull.notify_all();
    return 1;
}

template<typename T>
bool BlockingQueue<T>::isEmpty() {
    std::lock_guard<std::mutex> lockGuard(mutex);
    return queue.empty();
}

#endif //FFMPEGDEMO_BLOCKINGQUEUE_H
