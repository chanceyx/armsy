#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "acceptor.h"
#include "buffer.h"
#include "callbacks.h"
#include "eventloop.h"
#include "eventloop_threadpool.h"
#include "inet_address.h"
#include "noncpoyable.h"
#include "tcp_connection.h"

class TcpServer {
 public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop *loop, const InetAddress &listen_addr,
            const std::string &name, Option option = kNoReusePort);
  ~TcpServer();

  void setThreadInitCallback(const ThreadInitCallback &callback) {
    thread_init_callback_ = callback;
  }
  void setConnectionCallback(const ConnectionCallback &callback) {
    conn_callback_ = callback;
  }
  void setMessageCallback(const MessageCallback &callback) {
    msg_callback_ = callback;
  }
  void setWriteCompleteCallback(const WriteCompleteCallback &callback) {
    write_complete_callback_ = callback;
  }

  void setThreadPoolSize(int num);

  void start();

 private:
  void newConnection(int sockfd, const InetAddress &peer_addr);
  void removeConnection(const TcpConnectionPtr &conn_ptr);
  void removeConnectionInLoop(const TcpConnectionPtr &conn_ptr);

  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

  EventLoop *loop_;

  const std::string ip_port_;
  const std::string name_;

  std::unique_ptr<Acceptor> acceptor_;

  std::shared_ptr<EventLoopThreadPool> thread_pool_;

  ConnectionCallback conn_callback_;
  MessageCallback msg_callback_;
  WriteCompleteCallback write_complete_callback_;

  ThreadInitCallback thread_init_callback_;

  std::atomic_int started_;

  int next_conn_id_;
  ConnectionMap connections_;
};