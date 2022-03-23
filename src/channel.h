#pragma once

#include <functional>
#include <memory>

#include "noncpoyable.h"
#include "timestamp.h"

class EventLoop;

// Channel is a encapsulation of a socket fd, it provides
// functions to interact with socket fd.
class Channel : noncopyable
{
public:
    // Type define EventCallback & ReadEventCallback as a function
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // Handle a socket fd event
    void handleEvent(Timestamp receiveTime);

    // Set callback of socket fd events
    void setReadCallback(ReadEventCallback callback) { read_callback_ = std::move(callback); }
    void setWriteCallback(EventCallback callback) { write_callback_ = std::move(callback); }
    void setCloseCallback(EventCallback callback) { close_callback_ = std::move(callback); }
    void setErrorCallback(EventCallback callback) { error_callback_ = std::move(callback); }

    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void setRevents(int revents) { revents_ = revents; }

    // Set interest events of this channel(fd)
    void enableReadEvent();
    void disableReadEvent();
    void enableWriteEvent();
    void disableWriteEvent();
    void disableAllEvent();

    // Check if events_ is set.
    bool isWriteSet() const { return events_ & kWritable; }
    bool isReadSet() const { return events_ & kReadable; }

    // Check if events_ is set to 0.
    bool isSilent() const { return events_ == kNoneEvent; }

    // Get & Set the channel state.
    int getState() { return state_; }
    void setState(int st) { state_ = st; }

    // Get the eventloop related with this channel.
    EventLoop *getOwnerLoop() { return loop_; }

    // Remove this channel off it's poller
    void remove();

private:
    // Call the poller's updateChannel() function to update channel's state.
    void update();

    // TODO:
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kReadable;
    static const int kWritable;
    static const int kNoneEvent;

    EventLoop *loop_;
    const int fd_;

    // Interested events of a channel, in PollPoller and EpollPoller
    // events_ will be like POLLERR, POLLHUP, POLLIN, POLLOUT et al.
    int events_;

    // The events actually happened, in PollPoller and EpollPoller
    // events_ will be like POLLERR, POLLHUP, POLLIN, POLLOUT et al.
    int revents_;

    // A channel has three states: kChannelInitialized, kChannelAdded, 
    // kChannelRemoved.
    int state_;

    // TODO:
    std::weak_ptr<void> tie_;
    bool tied_;

    // Callback function of the triggered events.
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};
