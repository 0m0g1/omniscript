#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

// #include <queue>
// #include <mutex>
// #include <functional>
#include <omniscript/omniscript_pch.h>

class ThreadSafeQueue {
public:
    // Add a task to the back of the queue
    void push(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(std::move(task));
    }

    // Add a task to the front of the queue
    void pushToFront(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<std::function<void()>> tempQueue;
        tempQueue.push(std::move(task));

        // Move the existing tasks to the temporary queue
        while (!taskQueue_.empty()) {
            tempQueue.push(std::move(taskQueue_.front()));
            taskQueue_.pop();
        }

        // Swap back the queues
        std::swap(taskQueue_, tempQueue);
    }

    // Process all queued tasks
    void process() {
        std::queue<std::function<void()>> tasksToProcess;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(tasksToProcess, taskQueue_);
        }

        while (!tasksToProcess.empty()) {
            auto task = std::move(tasksToProcess.front());
            tasksToProcess.pop();
            task();
        }
    }

private:
    std::queue<std::function<void()>> taskQueue_; // Queue to store tasks
    std::mutex mutex_;                            // Mutex for thread safety
};

#endif // THREAD_SAFE_QUEUE_H
