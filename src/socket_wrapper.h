#pragma once

#include "noncpoyable.h"

class InetAddress;

// Socket is a wrapper of socket.
class Socket : noncopyable {
 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  // Get socket fd.
  int fd() const { return sockfd_; }

  // bind address of socket fd.
  void bindAddress(const InetAddress &local_addr);

  // TCP listen on socket fd. Call system call listen().
  void listen();

  // Accept connection. Call system call accept().
  int accept(InetAddress *peer_addr);

  // Shutdown socket fd's write channel. Call system call shutdown.
  void shutdownWrite();

  // Set TCP's option
  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

 private:
  // TCP listen socket fd.
  const int sockfd_;
};