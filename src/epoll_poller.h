#pragma once

#include <sys/epoll.h>
#include <vector>

#include "poller.h"
#include "timestamp.h"

class Channel;

// EpollPoller is a Poller using system call epoll_wait to poll.
class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    // Override the poll() function of Poller. It will eventually call epoll_wait.
    // The triggered events will store in events_.
    Timestamp poll(int timeout, ChannelList *active_channels) override;

    // Override the updateChannel() function of Poller. It will eventually call
    // epoll_ctl to add/modify the fd of the channel. This function would check
    // the channel's state. If the channel is initialzied, it will be added to
    // this epoll poller and the channel's state will be set to kChannelAdded.
    // If the channel's events_ is set to kNoneEvent which means it's interested
    // in no event, it will be remove from the epoll poller.
    void updateChannel(Channel *channel) override;

    // Override the removeChannel() function of Poller. It will eventually call
    // epoll_ctl to delete the fd of the channel from a epfd. This function would check
    // the channel's state. If the channel is added, it will be delete from
    // this epoll poller and the channel's state will be set to kChannelInitialzed.
    void removeChannel(Channel *channel) override;

private:
    static const int kDefaultChannelListSize = 16;

    // The triggerChannels() function put the triggered channels into
    // activeChannelList
    void triggerChannels(int num, ChannelList *active_channels) const;

    // Call epoll_ctl to add/del/modify the epitem(related to
    // the channel's fd) of the epoll_poller.
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    // The epoll fd of the epoller_poller.
    int epollfd_;

    // Trigger events of the epoll_poller will be stored in events_.
    EventList events_;
};
