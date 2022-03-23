#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "buffer.h"
#include "callbacks.h"
#include "inet_address.h"
#include "noncpoyable.h"
#include "timestamp.h"

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &local_addr, const InetAddress &peer_addr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &getName() const { return name_; }
  const InetAddress &getLocalAddr() const { return local_addr_; }
  const InetAddress &getPeerAddr() const { return peer_addr_; }

  bool isConnected() const { return state_ == kConnected; }

  void send(const std::string &buf);

  void shutdown();

  void connEstablish();
  void connDestory();

  void setConnectionCallback(const ConnectionCallback &callback) {
    conn_callback_ = callback;
  }
  void setMessageCallback(const MessageCallback &callback) {
    msg_callback_ = callback;
  }
  void setWriteCompleteCallback(const WriteCompleteCallback &callback) {
    write_complete_callback_ = callback;
  }
  void setCloseCallback(const CloseCallback &callback) {
    close_callback_ = callback;
  }
  void setHighWaterCallback(const HighWaterMarkCallback &callback) {
    highwater_mark_callback_ = callback;
  }

 private:
  enum ConnState {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting,
  };

  void setState(ConnState state) { state_ = state; }

  void handleRead(Timestamp receive_time);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();

  EventLoop *loop_;
  const std::string name_;
  std::atomic_int state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  ConnectionCallback conn_callback_;
  MessageCallback msg_callback_;
  WriteCompleteCallback write_complete_callback_;
  HighWaterMarkCallback highwater_mark_callback_;
  CloseCallback close_callback_;
  size_t high_water_mark_;

  Buffer input_buffer_;
  Buffer output_buffer_;
};