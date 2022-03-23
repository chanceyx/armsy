#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "noncpoyable.h"
#include "thread.h"

class EventLoop;

// EventLoopThread is a class wrap Thread and EventLoop.
class EventLoopThread : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;
  EventLoopThread(const ThreadInitCallback &callback = ThreadInitCallback(),
                  const std::string &name = std::string());

  // The destructor will set exiting_ to true and call thread_'s quit()
  // to stop the loop thread.
  ~EventLoopThread();

  // Start a eventloop thread and return the EventLoop. This function will
  // call std::thread to start a new thread. The new eventloop thread will
  // start looping and the eventloop object will be return in this thread.
  EventLoop *startLoop();

 private:
  // The thread function of thread_.
  void threadFunc();

  // EventLoop related to the thread.
  EventLoop *loop_;

  // If the eventloop thread is existing.
  bool exiting_;

  // A wrapped std::thread
  Thread thread_;

  // Lock used to protecting the global recourse of a process.
  std::mutex mutex_;
  std::condition_variable cond_;

  // A callback of a eventloop thread.
  ThreadInitCallback init_callback_;
};