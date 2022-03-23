#pragma once

#include <unordered_map>
#include <vector>

#include "noncpoyable.h"
#include "timestamp.h"

class Channel;
class EventLoop;

// Poller is a virtual base class of all the poller
// eg. epoll_poller/poll_poller/select_poller/kqueue_poller.
class Poller {
 public:
  // The ChannelList of the poller.
  using ChannelList = std::vector<Channel *>;

  Poller(EventLoop *loop);
  virtual ~Poller() = default;

  // The Poll() function will call the wait function(e.g. epoll_wait)
  // of the poller.
  virtual Timestamp poll(int timeout, ChannelList *active_channels) = 0;

  // TODO:
  // The updateChannel() function will call epoll_ctl to add/modify
  // the epitem(related to channel's fd) of the poller.
  virtual void updateChannel(Channel *channel) = 0;
  virtual void removeChannel(Channel *channel) = 0;

  // Check if the channel is added to the poller.
  bool isChannelExist(Channel *channel) const;

  // Return a epoll_poller as a default poller.
  static Poller *newDefaultPoller(EventLoop *loop);

 protected:
  using ChannelMap = std::unordered_map<int, Channel *>;

  // All channels added to the poller are stored in channels_.
  ChannelMap channels_;

 private:
  EventLoop *ownerloop_;
};