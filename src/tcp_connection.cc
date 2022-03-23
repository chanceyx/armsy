#include <errno.h>
#include <functional>
#include <netinet/tcp.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "channel.h"
#include "eventloop.h"
#include "logger.h"
#include "socket_wrapper.h"
#include "tcp_connection.h"

// TODO:
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &local_addr,
                             const InetAddress &peer_addr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop_, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runLoopCallback(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t wrote_len = 0;
    size_t remained_len = len;
    bool faultError = false;

    if (state_ = kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
    }

    if (!channel_->isWriteSet() && output_buffer_.readableBytes() == 0)
    {
        wrote_len = ::write(channel_->fd(), data, len);
        if (wrote_len >= 0)
        {
            remained_len = len - wrote_len;
            if (remained_len == 0 && write_complete_callback_)
            {
                // TODO: why share from this
                loop_->pushLoopCallback(std::bind(write_complete_callback_, shared_from_this()));
            }
        }
        else
        {
            wrote_len = 0;
            if (errno != EWOULDBLOCK)
            {
                // TODO: deal with fault
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remained_len > 0)
    {
        size_t remain_len_in_buffer = output_buffer_.readableBytes();
        if (remain_len_in_buffer + remained_len >= high_water_mark_ &&
            remain_len_in_buffer < high_water_mark_ &&
            highwater_mark_callback_)
        {
            loop_->pushLoopCallback(std::bind(highwater_mark_callback_, shared_from_this(),
                                              remain_len_in_buffer + remained_len));
        }
        output_buffer_.put((char *)data + wrote_len, remained_len);
        if (!channel_->isWriteSet())
        {
            channel_->enableWriteEvent();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runLoopCallback(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriteSet())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connEstablish()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReadEvent();

    conn_callback_(shared_from_this());
}

void TcpConnection::connDestory()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAllEvent();
        conn_callback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receive_time)
{
    int saved_errno = 0;
    ssize_t n = input_buffer_.read(channel_->fd(), &saved_errno);
    if (n > 0)
    {
        msg_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saved_errno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriteSet())
    {
        int save_errno = 0;
        ssize_t n = output_buffer_.write(channel_->fd(), &save_errno);
        if (n > 0)
        {
            output_buffer_.pushState(n);
            if (output_buffer_.readableBytes() == 0)
            {
                channel_->disableWriteEvent();
                if (write_complete_callback_)
                {
                    loop_->pushLoopCallback(std::bind(write_complete_callback_,
                                                      shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAllEvent();

    TcpConnectionPtr conn_ptr(shared_from_this());
    conn_callback_(conn_ptr);
    close_callback_(conn_ptr);
}

void TcpConnection::handleError()
{
    int opt;
    socklen_t opt_len = sizeof opt;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &opt, &opt_len) < 0)
    {
        err = errno;
    }
    else
    {
        err = opt;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}