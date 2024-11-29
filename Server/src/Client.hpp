#pragma once
#include <unistd.h>
#include <memory>
#include <vector>
#include "DataBuffer.hpp"
#include "Config.hpp"
#include "TLS.hpp"

class Client {
    const ID id_;
    Proto proto_;
    Socket socket_;
    SocketStatus status_;

    sockaddr_storage sockaddr_;
    socklen_t sock_len_;
    Port port_;
    Time timeout_;

    SSL_CTX* ctx_;
    bool under_tls_;
    std::string tls_data_;
    std::unique_ptr<TLS> tls_;

    ID NewID() {
        static ID id = 0;
        return id++;
    }

    std::vector<std::unique_ptr<DataBuffer>> msg_history_;
    std::vector<MsgStatus> msg_status_;
public:
    Client(Socket socket, TLS* tls = nullptr);
    Client(Port port, Time timeout, const sockaddr_storage& addr, socklen_t len);
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

    void AddMsg(std::unique_ptr<DataBuffer> msg, MsgStatus status);

    void UpdateTimeout(Time timeout) {
        timeout_ = timeout;
    }

    Time GetTimeout() {
        return timeout_;
    }

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

    size_t GetMsgCount() {
        return msg_history_.size();
    }

    const DataBuffer& GetMsg(size_t k) {
        return *msg_history_[k].get();
    }

    MsgStatus GetMsgStatus(size_t k) {
        return msg_status_[k];
    }

    bool SupportedTLS() const { return under_tls_; }
    std::string GetTLSData() {
        return std::move(tls_data_);
    }

    TLS* GetTLS() {
        return tls_.get();
    }
};