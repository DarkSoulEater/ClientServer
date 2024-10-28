#pragma once
#include <unistd.h>
#include <memory>
#include "DataBuffer.hpp"
#include "Config.hpp"

class Client {
    const ID id_;
    Proto proto_;
    Socket socket_;
    SocketStatus status_;

    sockaddr_storage sockaddr_;
    socklen_t sock_len_;
    Port port_;

    ID NewID() {
        static ID id = 0;
        return id++;
    }
public:
    Client(Socket socket);
    Client(Port port, const sockaddr_storage& addr, socklen_t len);
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

    Port GetPort() { // Undefined if client TCP
        return port_;
    }

    sockaddr* GetAddr() {
        return (sockaddr*)&sockaddr_;
    }

    socklen_t GetAddrLen() {
        return sock_len_;
    }
};