#include "Server.hpp"
#include <stdio.h>
#include <thread>
#include <format>
#include <string>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

Server::Status Server::GetStatus() {
    std::lock_guard<std::mutex> lock(status_mtx_);
    return status_;
}

void Server::SetStatus(Status status) {
    std::lock_guard<std::mutex> lock(status_mtx_);
    status_ = status;
}

bool Server::InitTLS() {
    ssl_ctx_ = SSL_CTX_new(TLS_method());
    if (!ssl_ctx_) {
        // perror();
        ERR_print_errors_fp(stderr);
        abort();
        return false;
    }


    if (SSL_CTX_use_certificate_file(ssl_ctx_, "./ca/server.crt", SSL_FILETYPE_PEM) != 1) {
        abort();
    }
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, "./ca/server.key", SSL_FILETYPE_PEM) != 1) {
        abort();
    } 

    if (SSL_CTX_check_private_key(ssl_ctx_) != 1) {
        perror("private key check");
        abort();
        return false;
    }

    // SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);
    SSL_CTX_set_options(ssl_ctx_, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
    // SSL_CTX_set_default_verify_paths(ssl_ctx_);

    encr_point_ = new EncryptPoint(ssl_ctx_, true);
    tls_.reset(new TLS(ssl_ctx_, true, &console_));
    return true;
}

void Server::IncTime() {
    std::lock_guard<std::mutex> lock(time_tmx_);
    ++time_;
}

Time Server::GetTime() {
    std::lock_guard<std::mutex> lock(time_tmx_);
    return time_;
}

Client *Server::FindClientByID(ID id) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        auto& client = *it->get();

        if (!client.IsValid())
            continue;;
    
        if (client.GetID() == id)
            return &client;
    }
    return nullptr;
}

Client *Server::FindHistoryClientByID(ID id) {
    std::lock_guard<std::mutex> lock(clients_history_mtx_);
    for (auto it = clients_history_.begin(); it != clients_history_.end(); ++it) {
        auto& client = *it->get();
    
        if (client.GetID() == id)
            return &client;
    }
    return nullptr;
}

Client *Server::FindClientByPort(Port port) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        auto& client = *it->get();

        if (!client.IsValid())
            continue;
    
        if (client.GetPort() == port)
            return &client;
    }
    return nullptr;
}

Client *Server::FindHistoryClientByPort(Port port) {
    std::lock_guard<std::mutex> lock(clients_history_mtx_);
    for (auto it = clients_history_.begin(); it != clients_history_.end(); ++it) {
        auto& client = *it->get();
    
        if (client.GetPort() == port)
            return &client;
    }
    return nullptr;
}

int Server::TCPInit() {
    serv_socket_ = tcp::Socket();
    if (serv_socket_ < 0) {
        perror("TCP Socket: ");
        return -1;
    }

    int bind_st = tcp::Bind(serv_socket_, port_);
    if (bind_st < 0) {
        perror("TCP Bind: ");
        return -1;
    }
    
    int listen_st = tcp::Listen(serv_socket_, 2);
    if (listen_st < 0) {
        perror("TCP Listen");
        return -1;
    }
    return 0;
}

int Server::UDPInit() {
    serv_socket_ = udp::Socket();
    if (serv_socket_ < 0) {
        perror("UDP Socket: ");
        return -1;
    }

    int bind_st = udp::Bind(serv_socket_, port_);
    if (bind_st < 0) {
        perror("UDP Bind: ");
        return -1;
    }
    return 0;
}

void Server::TCPSendTo(const std::string &msg, ID id) {
    auto* client_ = FindClientByID(id);
    if (client_ == nullptr) {
        console_.Log(std::format("client ID={} doesn't connect", id));
        return;
    }

    auto& client = *client_;
    Socket sock = client.GetSocket();

    if (sock < 0) {
        console_.Log(std::format("client ID={} does not connect", id));
        return;
    }

    // size_t size = msg.size() + 1;
    size_t size = msg.size();
    tcp::Send(sock, &size, sizeof(size_t), 0);
    tcp::Send(sock, msg.c_str(), size, 0);
    console_.Log(std::format("Msg[{}] send to {}", msg, id));
    auto data = std::make_unique<DataBuffer>(msg.size());
    client.AddMsg(std::make_unique<DataBuffer>(msg), MsgStatus::Server);
}

void Server::UDPSendTO(const std::string &msg, ID id) {
    auto* client_ = FindClientByID(id);
    if (client_ == nullptr) {
        console_.Log(std::format("client ID={} doesn't connect", id));
        return;
    }

    auto& client = *client_;

    size_t msg_size = msg.size(); // TODO: Packed in one Diagram
    int size_send_status = udp::SendTo(
        serv_socket_,
        &msg_size,
        sizeof(size_t),
        0,
        client.GetAddr(),
        client.GetAddrLen()
    );
    if (size_send_status != sizeof(size_t)) {
        perror("Send");
        return;
    }

    int send_status = udp::SendTo(
        serv_socket_,
        msg.c_str(),
        msg.size(),
        0,
        client.GetAddr(),
        client.GetAddrLen()
    );
    if (send_status != msg.size()) {
        perror("Send");
    } else {
        console_.Log(std::format("Msg[{}] send to {}", msg, id));
        client.AddMsg(std::make_unique<DataBuffer>(msg), MsgStatus::Server);
        client.UpdateTimeout(GetTime() + kLiveTime);
    }
}

void Server::TCPHandlingAcceptLoop() {
    if (proto_ == Proto::UDP)
        return;

    while (GetStatus() == Status::Up) {
        Socket client_sock = tcp::Accept(serv_socket_);
        if (client_sock < 0 || GetStatus() == Status::Close)
            continue;

        auto client = std::make_unique<Client>(client_sock);
        console_.Log(std::format("Connect new client: ID = {}", client.get()->GetID()));

        std::lock_guard<std::mutex> lock(clients_mtx_);
        clients_.emplace_back(std::move(client));
    }
}

void Server::TCPWaitingDataLoop() {
    if (proto_ == Proto::UDP)
        return;

    while (GetStatus() == Status::Up) {
        { // Lock client_mtx_
            std::lock_guard<std::mutex> lock(clients_mtx_);
            for (auto it = clients_.begin(); it != clients_.end(); ++it) {
                auto& client = *it->get();
                if (!client.IsValid()) {
                    std::thread([this, it]{Disconnect(it);}).detach();
                } else {
                    auto data_ = client.LoadData(); // Safe unique ptr
                    auto& data = *data_.get();

                    // if (!SSL_is_init_finished(encr_point_->ssl_)) {
                    //     console_.Log("HS not finished");
                    //     if (data.Size()) {
                    //         BIO_write(encr_point_->input_, data.Buffer(), data.Size());
                    //     }

                    //     std::string s;
                    //     console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
                    //     // int res = SSL_accept(encr_point_->ssl_);
                    //     int res = SSL_do_handshake(encr_point_->ssl_);
                    //     console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
                    //     auto err = SSL_get_error(encr_point_->ssl_, res);
                    //     if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    //         int n = 0;
                    //         char buf[2024];
                    //         do {    
                    //             n = BIO_read(encr_point_->output_, buf, sizeof(buf));
                    //         if (n > 0) {
                    //             s.append(buf, n);
                    //         } else if (!BIO_should_retry(encr_point_->output_))
                    //             console_.Log("HS READ ERROR");
                    //         } while (n>0);
                    //     }
                    //     std::thread([this, s, &client]{SendTo(s, client.GetID());}).detach();
                    //     continue;
                    // } else {
                    //     console_.Log("HS finished");
                    // }

                    if (tls_ && data.Size()) {
                        console_.Log("Decode");
                        auto want_send_data = tls_->Decode(data); // Decode must change data
                        console_.Log(std::format("{} \"{}\"", want_send_data.size(), want_send_data));
                        // SendTo(want_send_data, client.GetID());
                        if (!want_send_data.empty()) {
                            std::thread([this, want_send_data, &client]{SendTo(want_send_data, client.GetID());}).detach();
                        }
                    }

                    if (data.Size()) {
                        console_.Log(std::format("Recieved[{}]: \"{}\"", client.GetID(), data.Buffer()));
                        client.AddMsg(std::move(data_), MsgStatus::Client);
                    }
                }
            }
        } // End lock clients_mtx_
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Server::UDPWaitingDataLoop() {
    if (proto_ == Proto::TCP)
        return;
        
    while (GetStatus() == Status::Up) {
        sockaddr_storage addr;
        socklen_t socklen = sizeof(sockaddr_storage);

        auto data = UDPLoadData((sockaddr*)&addr, &socklen);

        // Find client
        char host[NI_MAXHOST], service[NI_MAXSERV];
        int getname_st = getnameinfo(
            (sockaddr*)&addr,
            socklen,
            host,
            NI_MAXHOST,
            service,
            NI_MAXSERV,
            NI_NUMERICSERV
        );

        if (data.get()->Size() && getname_st == 0) {
            // console_.Log(std::format("Recieve {} bytes from {}:{}. \"{}\"", data->Size(), host, service, data->Buffer()));

            Port port = std::stoi(service);
            auto* client = FindClientByPort(port);
            if (client == nullptr) {
                client = FindHistoryClientByPort(port);

                if (client != nullptr) {
                    Remember(client->GetID());
                }
            }

            if (client == nullptr) {
                auto client_ = std::make_unique<Client>(port, GetTime() + kLiveTime, addr, socklen);
                client = client_.get();
                console_.Log(std::format("Connect new client: ID = {}", client_->GetID()));

                std::lock_guard<std::mutex> lock(clients_mtx_);
                clients_.emplace_back(std::move(client_));
            }

            client->UpdateTimeout(GetTime() + kLiveTime);

            if (data->Size() == 0) {

            } else {
                console_.Log(std::format("Recieved[{}]: \"{}\"", client->GetID(), data->Buffer()));
                client->AddMsg(std::move(data), MsgStatus::Client);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        IncTime();
        CheckTimeOut();
    }
    
}

std::unique_ptr<DataBuffer> Server::UDPLoadData(sockaddr* sockaddr, socklen_t* socklen) {
    auto data = std::make_unique<DataBuffer>();

    size_t data_size = 0;
    int err = 0;
    int res = udp::RecvFrom(
        serv_socket_,
        &data_size, 
        sizeof(data_size), 
        MSG_DONTWAIT, 
        sockaddr,
        socklen
    );
    if (res == 0) {
        return data;
    } else if (res == -1) {
        socklen_t len = sizeof(err);
        getsockopt(serv_socket_, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
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
        // Fallthoughg
    case EAGAIN:
        return data;
    default:
        // std::cerr << "Unhandled error!\n"
        //     << "Code: " << err << " Err: " << std::strerror(err) << '\n';
        return data;
        break;
    }

    if (data_size == 0)
        return data;
    
    data.reset(new DataBuffer(data_size));
    udp::RecvFrom(
        serv_socket_,
        data.get()->Buffer(),
        data.get()->Size(), 
        MSG_DONTWAIT, 
        sockaddr,
        socklen
    );
    return data;
}

void Server::Disconnect(ClientList::iterator it) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    std::lock_guard<std::mutex> lock_histore(clients_history_mtx_);
    console_.Log(std::format("Client {} disconnected", it->get()->GetID()));
    clients_history_.push_back(std::move(*it));
    clients_.erase(it);
}

void Server::CheckTimeOut() {
    Time time = GetTime();
    clients_mtx_.lock();
    for (auto it = clients_.begin(); it != clients_.end();) {
        auto& client = *it->get();
        ++it;
    
        if (client.GetTimeout() < time) {
            clients_mtx_.unlock();
            Forget(client.GetID());
            clients_mtx_.lock();
        }
    }
    clients_mtx_.unlock();
}

void Server::Forget(ID id) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    std::lock_guard<std::mutex> his_lock(clients_history_mtx_);
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        auto& client = *it->get();
    
        if (client.GetID() == id) {
            clients_history_.push_back(std::move(*it));
            clients_.erase(it);
            console_.Log(std::format("Forget client {}", id));
            return;
        }
    }
}

void Server::Remember(ID id) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    std::lock_guard<std::mutex> his_lock(clients_history_mtx_);
    for (auto it = clients_history_.begin(); it != clients_history_.end(); ++it) {
        auto& client = *it->get();
    
        if (client.GetID() == id) {
            clients_.push_back(std::move(*it));
            clients_history_.erase(it);
            console_.Log(std::format("Remember client {}", id));
            return;
        }
    }
}

void Server::ConsoleLoop() {
    console_.Loop(commands_);
}

void Server::CommandLoop() {
    for (;;) {
        auto cmd_ = commands_.get();
        if (cmd_.has_value()) {
            auto& cmd = cmd_.value();
            switch (cmd.type) {
            case Command::Type::Exit: {
                Stop();
                exit(0);
                return;
            } break;

            case Command::Type::Send: {
                std::thread([this, &cmd]{SendTo(cmd.str, cmd.id_);}).detach();
            } break;

            case Command::Type::ShowClients: {
                std::thread([this]{PrintClients();}).detach();
            } break;

            case Command::Type::History: {
                std::thread([this]{PrintHistory();}).detach();
            } break;

            case Command::Type::Dialog: {
                std::thread([this, &cmd]{PrintMsgHistory(cmd.id_);}).detach();
            } break;

            default:
                console_.Print("[Err]: Unknow command type");
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Server::PrintClients() {
    console_.Print("Clients:");
    std::lock_guard<std::mutex> lock(clients_mtx_);
    if (clients_.empty()) {
        console_.Print("\tNone");
        return;
    }

    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        auto& client = *it->get();
        if (!client.IsValid()) {
            continue;
        } else {
            console_.Print("\t" + std::to_string(client.GetID()) + ": \n");
        }
    }
}

void Server::PrintHistory() {
    console_.Print("History clients:");
    std::lock_guard<std::mutex> lock(clients_history_mtx_);
    if (clients_history_.empty()) {
        console_.Print("\tNone");
        return;
    }

    for (auto it = clients_history_.begin(); it != clients_history_.end(); ++it) {
        auto& client = *it->get();
        console_.Print("\t" + std::to_string(client.GetID()) + ": \n");
    }
}

void Server::PrintMsgHistory(ID id) {
    auto* client_ = FindClientByID(id);
    if (client_ == nullptr) {
        client_ = FindHistoryClientByID(id);
    }
    if (client_ == nullptr) {
        console_.Log(std::format("Client {} doesn't exist", id));
        return;
    }
    auto& client = *client_;

    size_t msg_cnt = client.GetMsgCount();
    console_.Print(std::format("------------------------ Client ID = {} ------------------------", client.GetID()));
    for (size_t k = 0; k < msg_cnt; ++k) {
        MsgStatus msg_status = client.GetMsgStatus(k);
        auto& msg = client.GetMsg(k);
        console_.Print(
            std::format("{}: {}", 
                msg_status == MsgStatus::Client ? "Client" : "Server",
                msg.Buffer()
            )
        );
    }
}

int Server::Start() {
    int init_st = (proto_ == Proto::TCP ? TCPInit() : UDPInit());
    if (init_st < 0)
        return init_st;

    SetStatus(Status::Up);

    support_tls_ = InitTLS();

    auto tcp_accept_handler_thread_ = std::thread([this]{TCPHandlingAcceptLoop();});
    auto tcp_data_waiter_thread_    = std::thread([this]{TCPWaitingDataLoop();});
    auto udp_data_waiter_thread_    = std::thread([this]{UDPWaitingDataLoop();});
    auto console_loop_              = std::thread([this]{ConsoleLoop();});
    auto command_loop_              = std::thread([this]{CommandLoop();});

    console_.Log(std::format("Server started with protocol:{}, port:{}.", proto_ == Proto::TCP ? "TCP" : "UDP", port_));

    tcp_accept_handler_thread_.join();
    tcp_data_waiter_thread_.join();
    udp_data_waiter_thread_.join();
    console_loop_.join();
    command_loop_.join();
    return 0;
}

void Server::Stop() {
    SetStatus(Status::Close);
}

void Server::SendTo(const std::string& msg, ID id) {
    if (proto_ == Proto::TCP) {
        TCPSendTo(msg, id);
    } else {
        UDPSendTO(msg, id);
    }
}
