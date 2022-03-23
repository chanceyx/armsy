#include "tcp_server.h"

#include <string.h>

#include <functional>

#include "logger.h"
#include "tcp_connection.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop) {
  if (loop == nullptr) {
    LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &name, Option option)
    : loop_(CheckLoopNotNull(loop)),
      ip_port_(listenAddr.getIpAndPort()),
      name_(name),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      conn_callback_(),
      msg_callback_(),
      next_conn_id_(1),
      started_(0) {
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

TcpServer::~TcpServer() {
  for (auto &item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runLoopCallback(
        std::bind(&TcpConnection::connDestory, conn));
  }
}

void TcpServer::setThreadPoolSize(int num) { thread_pool_->setPoolSize(num); }

void TcpServer::start() {
  if (started_++ == 0) {
    thread_pool_->start(thread_init_callback_);
    loop_->runLoopCallback(std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn_ptr) {
  loop_->runLoopCallback(
      std::bind(&TcpServer::removeConnectionInLoop, this, conn_ptr));
}

void TcpServer::newConnection(int sockfd, const InetAddress &peer_addr) {
  EventLoop *io_loop = thread_pool_->getNextLoop();
  char buf[64] = {0};
  snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_conn_id_);

  // TODO: atomic ?
  ++next_conn_id_;

  std::string conn_name = name_ + buf;
  LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
           name_.c_str(), conn_name.c_str(), peer_addr.getIpAndPort().c_str());

  sockaddr_in local;
  ::memset(&local, 0, sizeof local);
  socklen_t addrlen = sizeof local;
  if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0) {
    LOG_ERROR("sockets::getLocalAddr");
  }

  InetAddress local_addr(local);
  TcpConnectionPtr conn_ptr(
      new TcpConnection(io_loop, conn_name, sockfd, local_addr, peer_addr));

  connections_[conn_name] = conn_ptr;

  conn_ptr->setConnectionCallback(conn_callback_);
  conn_ptr->setMessageCallback(msg_callback_);
  conn_ptr->setWriteCompleteCallback(write_complete_callback_);

  conn_ptr->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

  io_loop->runLoopCallback(std::bind(&TcpConnection::connEstablish, conn_ptr));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn_ptr) {
  LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
           name_.c_str(), conn_ptr->getName().c_str());

  connections_.erase(conn_ptr->getName());
  EventLoop *io_loop = conn_ptr->getLoop();
  io_loop->pushLoopCallback(std::bind(&TcpConnection::connDestory, conn_ptr));
}