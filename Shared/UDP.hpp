#pragma once
#include <sys/socket.h>
#include <netinet/in.h>

namespace udp {
    int Socket();

    int Bind(int sockfd, in_port_t port, in_addr_t sin_addr = INADDR_ANY);

    int Connect(int sockfd, in_port_t port, in_addr_t sin_addr = INADDR_LOOPBACK);

    ssize_t SendTo(int sockfd, const void *buf, size_t len, int flags, sockaddr *dest_addr, socklen_t addrlen);

    ssize_t RecvFrom(int sockfd, void *buf, size_t len, int flags, sockaddr *src_addr, socklen_t *addrlen);
}