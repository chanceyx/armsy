#include "poller.h"

#include "channel.h"

Poller::Poller(EventLoop *loop) : ownerloop_(loop) {}

bool Poller::isChannelExist(Channel *channel) const {
  auto it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}