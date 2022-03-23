#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <sys/eventfd.h>
#include <unistd.h>

#include "channel.h"
#include "default_poller.h"
#include "eventloop.h"
#include "logger.h"
#include "poller.h"

// Make sure there is only one eventloop per thread.
__thread EventLoop *t_loop_in_this_thread = nullptr;

// Default poll timewait is 10000ms
const int kPollWaitTime = 10000;

int createEventFd()
{
    int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0)
    {
        LOG_FATAL("create event fd error: %d\n", errno);
    }
    return event_fd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      calling_pending_functors_(false),
      thread_id_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeup_fd_(createEventFd()),
      wakeup_channel_(new Channel(this, wakeup_fd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, thread_id_);
    if (t_loop_in_this_thread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loop_in_this_thread, thread_id_);
    }
    else
    {
        t_loop_in_this_thread = this;
    }
    wakeup_channel_->setReadCallback(std::bind(&EventLoop::awake, this));
    wakeup_channel_->enableReadEvent();
}

EventLoop::~EventLoop()
{
    wakeup_channel_->disableAllEvent();
    wakeup_channel_->remove();
    ::close(wakeup_fd_);
    t_loop_in_this_thread == nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);

    while (!quit_)
    {
        active_channels_.clear();
        poll_return_time_ = poller_->poll(kPollWaitTime, &active_channels_);
        for (Channel *channel : active_channels_)
        {
            channel->handleEvent(poll_return_time_);
        }
        executePendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runLoopCallback(Functor callback)
{
    if (isInLoopThread())
    {
        callback();
    }
    else
    {
        pushLoopCallback(callback);
    }
}

void EventLoop::pushLoopCallback(Functor callback)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pending_functors_.emplace_back(callback);
    }

    if (!isInLoopThread() || calling_pending_functors_)
    {
        wakeup();
    }
}

void EventLoop::awake()
{
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::isChannelExist(Channel *channel)
{
    poller_->isChannelExist(channel);
}

void EventLoop::executePendingFunctors()
{
    std::vector<Functor> functors;
    calling_pending_functors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }

    calling_pending_functors_ = false;
}