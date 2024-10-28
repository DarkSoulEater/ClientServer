#pragma once
#include <unistd.h>
#include <memory>
#include "DataBuffer.hpp"
typedef int Socket;
typedef size_t ID;

enum class SocketStatus {
    Connected,
    Disconnected
};

class Client {
    const ID id_;
    Socket socket_;
    SocketStatus status_;

    ID NewID() {
        static ID id = 0;
        return id++;
    }

public:
    Client(Socket socket) : socket_(socket), status_(SocketStatus::Connected), id_(NewID()) {}
    ~Client() {
        if (socket_ >= 0) {
            close(socket_);
        }
    }

    Socket GetSocket() { return socket_; }
    void CloseSocket() {
        if (socket_ >= 0) { 
            close(socket_); 
            socket_ = -1;
        }
        status_ = SocketStatus::Disconnected;
    }

    bool IsValid();
    std::unique_ptr<DataBuffer> LoadData();

    ID GetID() {
        return id_;
    }
};