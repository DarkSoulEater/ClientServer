#pragma once
#include <sys/socket.h>
#include <netinet/in.h>

namespace udp {
    int Socket();

    int Bind(int sockfd, in_port_t port, in_addr_t sin_addr = INADDR_ANY);

    // int Listen(int sockfd, int n);

    int Connect(int sockfd, in_port_t port, in_addr_t sin_addr = INADDR_LOOPBACK);

    // int Accept(int sockfd);

    ssize_t Send(int sockfd, void *buf, size_t len, int flags);

    ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
}