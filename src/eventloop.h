#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "current_thread.h"
#include "noncpoyable.h"
#include "timestamp.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // Start looping, the poller_ of the eventloop will call poll() to wait
    // until events happened. When a loop started, looping_ will be set to
    // true and quit_ will be set to false. doPendingFunctors() will be called
    // in each round of loop.
    void loop();

    // Quit loopping. if the current thread is not a event loop thread, wake
    // the event loop thread up.
    void quit();

    // Get the return time of the poller_'s poll().
    Timestamp pollReturnTime() const { return poll_return_time_; }

    // Call the callback of an event loop. If the current thread is not a event
    // loop thread, if this thread is not the event loop thread, than call
    // pushLoopCallback() tu push the callback into a queue.
    void runLoopCallback(Functor callback);

    // Push the callback into a queue and wake the event loop up.
    void pushLoopCallback(Functor callback);

    // Multi eventloop looping threads might run in a same process. And they
    // might be sleeping at poller_'s poll().This function used to Wake the
    // other eventloop up.
    void wakeup();

    // Call poller_'s updateChannel() to update/remove a channel from poller.
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    // Check if the channel is in poller_.
    bool isChannelExist(Channel *channel);

    // If the current thread running a event loop.
    bool isInLoopThread() const { return thread_id_ == CurrentThread::tid(); }

private:
    // The eventloop sleeping at poller_'s poll() could wake up from this
    // function.
    void awake();

    // Execute the pending functors.
    void executePendingFunctors();

    using ChannelList = std::vector<Channel *>;

    // If the eventloop loping.
    std::atomic_bool looping_;

    // If the eventloop is quit.
    std::atomic_bool quit_;

    // The eventloop's tid.
    const pid_t thread_id_;

    // The return time of poller_'s poll().
    Timestamp poll_return_time_;

    // A poller of an EventLoop.
    std::unique_ptr<Poller> poller_;

    // A eventfd used to wake the other eventloop up.
    int wakeup_fd_;

    // A channel of wakeup_fd_.
    std::unique_ptr<Channel> wakeup_channel_;

    // Active channels of a round of looping.
    ChannelList active_channels_;

    // If the eventloop is calling pending functors.
    std::atomic_bool calling_pending_functors_;

    // The eventloop's pending functors.
    std::vector<Functor> pending_functors_;

    std::mutex mutex_;
};