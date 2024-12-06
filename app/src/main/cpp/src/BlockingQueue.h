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
    std::condition_variable notFull;
    int capacity;
    std::atomic<bool> isShutdown;
public:
    explicit BlockingQueue(int capacity = INT_MAX): capacity(capacity), isShutdown(false) {};
    void enqueue(const T &item);
    T dequeue();
    bool isEmpty();
    void shutdown();
};

template<typename T>
void BlockingQueue<T>::shutdown() {
    isShutdown = true;
    notFull.notify_all();
    notEmpty.notify_all();
}

template<typename T>
void BlockingQueue<T>::enqueue(const T &item) {
    std::unique_lock<std::mutex> lock(mutex);
    notFull.wait(lock, [this] {return isShutdown || queue.size() < capacity;});
    if (isShutdown && queue.size() >= capacity) {
        throw std::runtime_error("BlockingQueue is shutdown");
    }
    queue.push(item);
    notEmpty.notify_all();
}

template<typename T>
T BlockingQueue<T>::dequeue() {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this]{ return isShutdown || !queue.empty();});
    if (isShutdown && queue.empty()) {
        throw std::runtime_error("BlockingQueue is shutdown");
    }
    T item = queue.front();
    queue.pop();
    notFull.notify_all();
    return item;
}

template<typename T>
bool BlockingQueue<T>::isEmpty() {
    std::lock_guard<std::mutex> lockGuard(mutex);
    return queue.empty();
}

#endif //FFMPEGDEMO_BLOCKINGQUEUE_H
