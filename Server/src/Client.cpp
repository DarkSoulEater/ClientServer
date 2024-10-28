#include "Client.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include "TCP.hpp"

bool Client::IsValid() {
    return socket_ >= 0 && status_ == SocketStatus::Connected;
}

std::unique_ptr<DataBuffer> Client::LoadData() {
    auto data = std::make_unique<DataBuffer>();
    if (status_ == SocketStatus::Disconnected)
        return data;

    size_t data_size = 0;
    int err = 0;
    int res = tcp::Recv(socket_, &data_size, sizeof(data_size), MSG_DONTWAIT);
    if (res == 0) {
        CloseSocket();
        return data;
    } else if (res == -1) {
        socklen_t len = sizeof(err);
        getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
        if (!err) {
            err = errno;
        }
    }

    switch (err) {
    case 0:
        break;
    case ETIMEDOUT:
    case ECONNRESET:
    case EPIPE:
        CloseSocket();
        // Fallthoughg
    case EAGAIN:
        return data;
    default:
        CloseSocket();
        std::cerr << "Unhandled error!\n"
            << "Code: " << err << " Err: " << std::strerror(err) << '\n';
        return data;
        break;
    }

    if (data_size == 0)
        return data;
    
    data.reset(new DataBuffer(data_size));
    tcp::Recv(socket_, data.get()->Buffer(), data.get()->Size(), 0);
    return data;
}