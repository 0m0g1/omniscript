#ifndef MAIN_THREAD_EVENT_RUNNER_H
#define MAIN_THREAD_EVENT_RUNNER_H

// #include <queue>
// #include <mutex>
// #include <functional>
#include <omniscript/omniscript_pch.h>

class MainThreadEventRunner {
public:
    static MainThreadEventRunner& getInstance() {
        static MainThreadEventRunner instance;
        return instance;
    }

    // Add an event to be run on the main thread
    void addEvent(std::function<void()> event) {
        std::lock_guard<std::mutex> lock(mutex);
        eventQueue.push(std::move(event));
    }

    // Process all queued events on the main thread
    void processEvents() {
        std::queue<std::function<void()>> eventsToProcess;

        {
            std::lock_guard<std::mutex> lock(mutex);
            std::swap(eventsToProcess, eventQueue); // Transfer events to a local queue
        }

        // Execute all events
        while (!eventsToProcess.empty()) {
            auto event = std::move(eventsToProcess.front());
            eventsToProcess.pop();
            event();
        }
    }

private:
    MainThreadEventRunner() = default;

    std::queue<std::function<void()>> eventQueue;
    std::mutex mutex;
};

#define MAIN_THREAD_RUNNER MainThreadEventRunner::getInstance()
#define ADD_MAIN_THREAD_EVENT(event) MAIN_THREAD_RUNNER.addEvent(event)
#define PROCESS_MAIN_THREAD_EVENTS() MAIN_THREAD_RUNNER.processEvents()

#endif // MAIN_THREAD_EVENT_RUNNER_H
