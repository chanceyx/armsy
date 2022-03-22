#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// InetAddress is a wrapper of socket.
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    // Get number-and-dot IP address, e.g. "192.168.1.1".
    std::string getNumberAndDotIp() const;

    // Get <ip>:<port> format string, e.g. "192.168.1.1:80".
    std::string getIpAndPort() const;

    // Get port.
    uint16_t getShortIntPort() const;

    // Get sockaddr_in format sockaddr.
    const sockaddr_in *getSockAddr() const { return &addr_; }

    // Set sockaddr.
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    // Sockaddr.
    sockaddr_in addr_;
};