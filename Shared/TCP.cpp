#include "TCP.hpp"
#include <cstddef>

int tcp::Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

int tcp::Bind(int sockfd, in_port_t port, in_addr_t sin_addr) {
    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr   = htonl(sin_addr)
    };
    return bind(sockfd, (const sockaddr*)&addr, sizeof(addr));
}

int tcp::Listen(int sockfd, int n) {
    return listen(sockfd, n);
}

int tcp::Connect(int sockfd, in_port_t port, in_addr_t sin_addr) {
    sockaddr_in addr {
        .sin_family = AF_INET,
        .sin_port   = htons(port),
        .sin_addr   = htonl(sin_addr)
    };
    return connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
}

int tcp::Accept(int sockfd) {
    return accept(sockfd, NULL, NULL);
}

ssize_t tcp::Send(int sockfd, const void *buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

ssize_t tcp::Recv(int sockfd, void *buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
}
