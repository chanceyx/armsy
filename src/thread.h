#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

#include "noncpoyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    // Start a new thread and get it's tid. Since the new thread's started
    // started_ will be set to true.
    void start();

    // The function thead_.join() will be called to join a thread.
    void join();

    // If a thread is started
    bool isStarted() { return started_; }

    // If a thread is joined.
    bool isJoined() { return joined_; }

    // Get thread_'s tid.
    pid_t tid() const { return tid_; }

    // Get Thread'a name.
    const std::string &name() const { return name_; }

    // Return the number of the running thread .
    static int countThread() { return thread_num_; }

private:
    // Set thread's default name.
    void setDefaultName();

    // If the thread is started.
    bool started_;

    // If the thread is joined.
    bool joined_;

    // Hold the pointer of real thread.
    std::shared_ptr<std::thread> thread_;

    // Thread's id.
    pid_t tid_;

    // Function the thread will be executing.
    ThreadFunc func_;

    // Thread's name.
    std::string name_;

    // The number of running thread.
    static std::atomic_int thread_num_;
};
