#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "acceptor.h"
#include "inet_address.h"
#include "logger.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuseprot)
    : loop_(loop),
      accept_socket_(createNonblocking()),
      accept_channel_(loop, accept_socket_.fd()),
      listenning_(false)
{
    accept_socket_.setReuseAddr(true);
    accept_socket_.setReusePort(true);
    accept_socket_.bindAddress(listen_addr);
    accept_channel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    accept_channel_.disableAllEvent();
    accept_channel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    accept_socket_.listen();
    accept_channel_.enableReadEvent();
}

void Acceptor::handleRead()
{
    InetAddress peer_addr;
    int conn_fd = accept_socket_.accept(&peer_addr);
    if (conn_fd >= 0)
    {
        if (new_connection_callback_)
        {
            new_connection_callback_(conn_fd, peer_addr);
        }
        else
        {
            ::close(conn_fd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}