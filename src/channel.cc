#include <sys/epoll.h>

#include "channel.h"
#include "eventloop.h"
#include "logger.h"

const int Channel::kReadable = EPOLLIN | EPOLLPRI;
const int Channel::kWritable = EPOLLOUT;
const int Channel::kNoneEvent = 0;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), state_(-1), tied_(false) {}

Channel::~Channel() {}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::disableAllEvent()
{
    events_ = kNoneEvent;
    update();
}

void Channel::enableReadEvent()
{
    events_ |= kReadable;
    update();
}

void Channel::disableReadEvent()
{
    events_ &= ~kReadable;
    update();
}

void Channel::enableWriteEvent()
{
    events_ |= kWritable;
    update();
}

void Channel::disableWriteEvent()
{
    events_ &= ~kWritable;
    update();
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvents revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        LOG_INFO("XXXXXXXXXXXXXXX\n\n\n");
        if (close_callback_)
        {
            close_callback_();
        }

        // if (revents_ & EPOLLERR)
        // {
        //     if (error_callback_)
        //     {
        //         error_callback_();
        //     }
        // }

        // if (revents_ & EPOLLIN | EPOLLPRI)
        // {
        //     if (read_callback_)
        //     {
        //         read_callback_(receiveTime);
        //     }
        // }

        // if (revents_ & EPOLLOUT)
        // {
        //     if (write_callback_)
        //     {
        //         write_callback_();
        //     }
        // }
    }

    if (revents_ & EPOLLERR)
    {
        if (error_callback_)
        {
            error_callback_();
        }
    }

    if (revents_ & EPOLLIN | EPOLLPRI)
    {
        if (read_callback_)
        {
            read_callback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (write_callback_)
        {
            write_callback_();
        }
    }
}