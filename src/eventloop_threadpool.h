#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "noncpoyable.h"

class EventLoop;
class EventLoopThread;

// EventLoopThreadPool is a thread pool of EventLoopThread.
class EventLoopThreadPool : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *baseLoop, const std::string &pool_name);
  ~EventLoopThreadPool();

  // Set the thread pool size.
  void setPoolSize(int thread_num) { thread_num_ = thread_num; }

  // Start all threads of the pool.
  void start(const ThreadInitCallback &callback = ThreadInitCallback());

  // Get the next eventloop by robin.
  EventLoop *getNextLoop();

  // Get all the eventloop of this pool.
  std::vector<EventLoop *> getAllLoops();

  // Is the threads of the pool start looping.
  bool isStarted() const { return started_; }

  // Get the pool name.
  const std::string name() const { return name_; }

 private:
  // Main reactor(main eventloop) of this pool.
  EventLoop *baseLoop_;

  // Pool name.
  std::string name_;

  // If the threads of the pool start looping.
  bool started_;

  // Pool size.
  int thread_num_;

  // Next loop.
  int next_;

  // All the eventloop threads of the pool.
  std::vector<std::unique_ptr<EventLoopThread>> threads_;

  // All the eventloops of the pool.
  std::vector<EventLoop *> loops_;
};
