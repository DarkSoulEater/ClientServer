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

ssize_t udp::Send(int sockfd, void *buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

ssize_t udp::Recv(int sockfd, void *buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
}
