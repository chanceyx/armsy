#include <string.h>
#include <strings.h>

#include "inet_address.h"

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;

    // convert host to network byte order(short).
    addr_.sin_port = ::htons(port);

    // Convert Internet host address from numbers-and-dots notation
    // into binary data in network byte order.
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

std::string InetAddress::getNumberAndDotIp() const
{
    char buf[64] = {0};

    // Convert binary data in network byte order to numbers-and-dots notation
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

uint16_t InetAddress::getShortIntPort() const
{
    return ::ntohs(addr_.sin_port);
}

std::string InetAddress::getIpAndPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}