#pragma once

#include <functional>

#include "channel.h"
#include "noncpoyable.h"
#include "socket_wrapper.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuseport);
    ~Acceptor();

    // Set callback of new connection.
    void setNewConnectionCallback(const NewConnectionCallback &callback) { new_connection_callback_ = callback; }

    // If acceptor is listenning.
    bool isListenning() const { return listenning_; }

    // Listen on accpet_socket_.
    void listen();

private:
    void handleRead();

    EventLoop *loop_;
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listenning_;
};