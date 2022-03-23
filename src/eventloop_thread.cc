#include "eventloop_thread.h"
#include "eventloop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &callback,
                                 const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      init_callback_(callback)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop ev_loop;

    if (init_callback_)
    {
        init_callback_(&ev_loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &ev_loop;
        cond_.notify_one();
    }

    ev_loop.loop();
    std::unique_lock<std::mutex> loack(mutex_);
    loop_ = nullptr;
}