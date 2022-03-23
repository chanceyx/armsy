#pragma once
#include <stdlib.h>

#include "epoll_poller.h"
// #include "kqueue_poller.h"
// #include "poll_poller.h"
#include "poller.h"
// #include "select_poller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop) {
  if (::getenv("USE_POLL")) {
    return nullptr;
  } else if (::getenv("USE_SELECT")) {
    return nullptr;
  } else if (::getenv("USE_KQUEUE")) {
    return nullptr;
  } else {
    return new EpollPoller(loop);
  }
  return new EpollPoller(loop);
}