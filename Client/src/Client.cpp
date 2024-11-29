#include "Client.hpp"
#include <format>
#include "TCP.hpp"
#include "UDP.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "tls/TLS.hpp"

int Client::TCPInit() {
    clinent_sock_ = tcp::Socket();
    if (clinent_sock_ < 0) {
        perror("TCP Socket");
        return -1;
    }

    int connect_st = tcp::Connect(clinent_sock_, port_, serv_ip_);
    if (connect_st < 0) {
        perror("TCP Connect");
        return -1;
    }
    return 0;
}

int Client::UDPInit() {
    clinent_sock_ = udp::Socket();
    if (clinent_sock_ < 0) {
        perror("UDP Socket");
        return -1;
    }

    int connect_st = tcp::Connect(clinent_sock_, port_, serv_ip_);
    if (connect_st < 0) {
        perror("UDP Connect");
        return -1;
    }
    return 0;
}

Client::Status Client::GetStatus() {
    std::lock_guard<std::mutex> lock(status_mtx_);
    return status_;
}

void Client::SetStatus(Status status) {
    std::lock_guard<std::mutex> lock(status_mtx_);
    status_ = status;
}

void Client::WaitingDataLoop() {
    while (GetStatus() == Status::Up) {
        auto data_ = (proto_ == Proto::TCP ? TCPLoadData() : UDPLoadData()); // Safe unique ptr
        auto& data = *data_.get();
        if (under_tls_ && data.Size()) {
            auto want_send_data = tls_->Decode(data);
            if (!want_send_data.empty()) {
                Send(want_send_data);
            }
        }
        if (data.Size()) {
            console_.Log(std::format("Server: \"{}\"", data.Buffer()));
        }
 
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Client::ConsoleLoop() {
    console_.Loop(commands_, Device::Client);
}

void Client::CommandLoop() {
    for (;;) {
        auto cmd_ = commands_.get();
        if (cmd_.has_value()) {
            auto& cmd = cmd_.value();

            switch (cmd.type) {
            case Command::Type::Exit: {
                Stop();
                return;
            } break;

            case Command::Type::Send: {
                if (under_tls_) {
                    tls_->Encode(cmd.str);
                }
                std::thread([this, &cmd]{Send(cmd.str);}).detach();
            } break;

            default:
                console_.Print("[Err]: Unknow command type");
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

std::unique_ptr<DataBuffer> Client::TCPLoadData() {
    auto data = std::make_unique<DataBuffer>();

    size_t data_size = 0;
    int err = 0;
    int res = tcp::Recv(clinent_sock_, &data_size, sizeof(data_size), MSG_DONTWAIT);
    if (res == 0) {
        Stop();
        return data;
    } else if (res == -1) {
        socklen_t len = sizeof(err);
        getsockopt(clinent_sock_, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
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
        Stop();
        // Fallthoughg
    case EAGAIN:
        return data;
    default:
        Stop();
        return data;
        break;
    }

    if (data_size == 0)
        return data;
    
    data.reset(new DataBuffer(data_size));

    ssize_t recv_size = 0;
    do {
        ssize_t recv_count = tcp::Recv(clinent_sock_, data.get()->Buffer(), data.get()->Size(), 0);
        if (recv_count > 0) {
            recv_size += recv_count;
        }
    } while(recv_size < data_size);
    return data;
}

std::unique_ptr<DataBuffer> Client::UDPLoadData() {
    auto data = std::make_unique<DataBuffer>();

    size_t data_size = 0;
    int err = 0;
    sockaddr_storage addr;
    socklen_t socklen = sizeof(sockaddr_storage);

    int res = udp::RecvFrom(
        clinent_sock_,
        &data_size,
        sizeof(data_size),
        MSG_DONTWAIT,
        (sockaddr*)&addr,
        &socklen
    );
    if (res == 0) {
        Stop();
        return data;
    } else if (res == -1) {
        socklen_t len = sizeof(err);
        getsockopt(clinent_sock_, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
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
        Stop();
        // Fallthoughg
    case EAGAIN:
        return data;
    default:
        Stop();
        return data;
        break;
    }

    if (data_size == 0)
        return data;
    
    data.reset(new DataBuffer(data_size));
    udp::RecvFrom(
        clinent_sock_,
        data.get()->Buffer(),
        data.get()->Size(),
        0,
        (sockaddr*)&addr,
        &socklen
    );
    return data;
}

bool Client::InitTLS() {
    if (!need_tls_) {
        return false;
    }

    if (proto_ == Proto::UDP) {
        console_.Log("TLS supported only for TCP");
        need_tls_ = false;
        return false;
    }

    static SSL_CTX* ctx_ = SSL_CTX_new(TLS_method());
    if (!ctx_) {
        perror("create ctx");
        ERR_print_errors_fp(stderr);
        abort();
    }

    // SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
    SSL_CTX_set_options(ctx_, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
    // SSL_CTX_set_default_verify_paths(ctx_);

    // encr_point_ = new EncryptPoint(ctx_, false);
    tls_.reset(new TLS(ctx_, false, &console_));
    auto data = tls_->Handshake();
    if (!data.empty()) {
        Send(data);
    }
    return true;
}

// bool Client::HandShake() {
//     console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
//     int res = SSL_do_handshake(encr_point_->ssl_);
//     console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));

//     auto status = SSLGetStatus(encr_point_->ssl_, res);
//     if (status == SSLStatus::WantIO) {
//         auto data = BIORead(encr_point_->output_);
//         Send(data);
//     }
//     return status;
// }

int Client::Start() {
    int init_st = (proto_ == Proto::TCP ? TCPInit() : UDPInit());
    if (init_st < 0) {
        return init_st;
    }

    SetStatus(Status::Up);

    under_tls_ = InitTLS();

    auto data_waiter_thread_ = std::thread([this]{WaitingDataLoop();});
    auto console_loop_       = std::thread([this]{ConsoleLoop();});
    auto command_loop_       = std::thread([this]{CommandLoop();});

    console_.Log(std::format(
        "Client started with protocol:{}, port:{}, server_ip:{}.{}.{}.{}"
      , proto_ == Proto::TCP ? "TCP" : "UDP"
      , port_
      , (serv_ip_ & 0xff000000) >> 24, (serv_ip_ & 0x00ff0000) >> 16, (serv_ip_ & 0x0000ff00) >> 8, (serv_ip_ & 0x000000ff)
    ));

    data_waiter_thread_.join();
    console_loop_.join();
    command_loop_.join();

    return 0;
}

void Client::Stop() {
    exit(0);
}

void Client::Send(const std::string &msg) {
    // size_t size = msg.size() + 1;
    size_t size = msg.size();
    send(clinent_sock_, &size, sizeof(size_t), 0);
    send(clinent_sock_, msg.c_str(), size, 0);
    console_.Log(std::format("[{}] send to server", msg));
}
