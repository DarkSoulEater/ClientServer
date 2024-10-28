#include "UDP.hpp"
#include <cstddef>

int udp::Socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int udp::Bind(int sockfd, in_port_t port, in_addr_t sin_addr) {
    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr   = htonl(sin_addr)
    };
    return bind(sockfd, (const sockaddr*)&addr, sizeof(addr));
}

int udp::Connect(int sockfd, in_port_t port, in_addr_t sin_addr) {
    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr   = htonl(sin_addr)
    };
    return connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
}

ssize_t udp::SendTo(int sockfd, const void *buf, size_t len, int flags, sockaddr *dest_addr, socklen_t addrlen) {
    return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t udp::RecvFrom(int sockfd, void *buf, size_t len, int flags, sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}
