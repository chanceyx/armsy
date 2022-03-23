#include "epoll_poller.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "channel.h"
#include "logger.h"

const int kChannelInitialized = -1;
const int kChannelAdded = 1;
const int kChannelRemoved = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kDefaultChannelListSize) {
  if (epollfd_ < 0) {
    LOG_FATAL("epoll_create error:%d \n", errno);
  }
}

EpollPoller::~EpollPoller() { ::close(epollfd_); }

Timestamp EpollPoller::poll(int timeout, ChannelList *activeChannels) {
  LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());
  int num_events = ::epoll_wait(epollfd_, &*events_.begin(),
                                static_cast<int>(events_.size()), timeout);
  int err = errno;
  Timestamp now_state(Timestamp::now());

  if (num_events > 0) {
    LOG_INFO("%d events happend\n", num_events);
    triggerChannels(num_events, activeChannels);
    if (num_events == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (num_events == 0) {
    LOG_DEBUG("%s timeout!\n", __FUNCTION__);
  } else {
    if (err != EINTR) {
      errno = err;
      LOG_ERROR("EpollPoller::poll() error!");
    }
  }
  return now_state;
}

void EpollPoller::updateChannel(Channel *channel) {
  const int st = channel->getState();
  LOG_INFO("finc=%s => fd=%d events=%d state=%d\n", __FUNCTION__, channel->fd(),
           channel->events(), st);
  if (st == kChannelInitialized || st == kChannelRemoved) {
    if (st == kChannelInitialized) {
      int fd = channel->fd();
      // TODO: check if channel is already in channels_
      channels_[fd] = channel;
    } else {
      // TODO: check if channel is still in channels_ and its fd still match
    }
    channel->setState(kChannelAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    int fd = channel->fd();
    if (channel->isSilent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->setState(kChannelRemoved);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::removeChannel(Channel *channel) {
  int fd = channel->fd();
  channels_.erase(fd);

  LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

  int st = channel->getState();
  if (st == kChannelAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->setState(kChannelInitialized);
}

void EpollPoller::triggerChannels(int num, ChannelList *active_channels) const {
  for (int i = 0; i < num; i++) {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->setRevents(events_[i].events);
    active_channels->push_back(channel);
  }
}

void EpollPoller::update(int operation, Channel *channel) {
  epoll_event ep_event;
  ::memset(&ep_event, 0, sizeof(ep_event));

  int fd = channel->fd();

  ep_event.events = channel->events();
  ep_event.data.fd = fd;
  ep_event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, fd, &ep_event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_ERROR("epoll_ctl del error:%d\n", errno);
    } else {
      LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
    }
  }
}