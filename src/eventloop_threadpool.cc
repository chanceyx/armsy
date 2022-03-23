#include "eventloop_threadpool.h"

#include <memory>

#include "eventloop_thread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,
                                         const std::string &pool_name)
    : baseLoop_(baseLoop),
      name_(pool_name),
      started_(false),
      thread_num_(0),
      next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {
  //
}

void EventLoopThreadPool::start(const ThreadInitCallback &callback) {
  started_ = true;

  for (int i = 0; i < thread_num_; i++) {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    EventLoopThread *t = new EventLoopThread(callback, buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startLoop());  // start looping
  }

  if (thread_num_ == 0 && callback) {
    callback(baseLoop_);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  EventLoop *loop = baseLoop_;

  if (!loops_.empty()) {
    loop = loops_[next_];
    next_ = (next_ + 1) % loops_.size();
  }
  return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
  if (!loops_.empty()) {
    return std::vector<EventLoop *>(1, baseLoop_);
  } else {
    return loops_;
  }
}