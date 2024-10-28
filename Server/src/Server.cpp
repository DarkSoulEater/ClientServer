#include "Server.hpp"
#include <stdio.h>
#include <thread>
#include <format>
#include <string>

Server::Status Server::GetStatus() {
    std::lock_guard<std::mutex> lock(status_mtx_);
    return status_;
}

void Server::SetStatus(Status status) {
    std::lock_guard<std::mutex> lock(status_mtx_);
    status_ = status;
}

void Server::HandlingAcceptLoop() {
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

void Server::WaitingDataLoop() {
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

                    if (data.Size()) {
                        console_.Log(std::format("Recieved[{}]: \"{}\"", client.GetID(), data.Buffer()));
                    }
                }
            }
        } // End lock clients_mtx_
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Server::Disconnect(ClientList::iterator it) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    std::lock_guard<std::mutex> lock_histore(clients_history_mtx_);
    console_.Log(std::format("Client {} disconnected", it->get()->GetID()));
    clients_history_.push_back(std::move(*it));
    clients_.erase(it);
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

int Server::Start() {
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

    SetStatus(Status::Up);

    auto accept_handler_thread_ = std::thread([this]{HandlingAcceptLoop();});
    auto data_waiter_thread_    = std::thread([this]{WaitingDataLoop();});
    auto console_loop_          = std::thread([this]{ConsoleLoop();});
    auto command_loop_          = std::thread([this]{CommandLoop();});

    console_.Log(std::format("Server started with protocol:{}, port:{}.", proto_ == Proto::TCP ? "TCP" : "UDP", port_));

    accept_handler_thread_.join();
    data_waiter_thread_.join();
    console_loop_.join();
    command_loop_.join();
    return 0;
}

void Server::Stop() {
    SetStatus(Status::Close);
}

void Server::SendTo(const std::string& msg, ID id) {
    Socket sock = -1;
    {
        std::lock_guard<std::mutex> lock(clients_mtx_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            auto& client = *it->get();
            console_.Log(std::format("{}", client.GetID()));
            if (client.GetID() == id) {
                sock = client.GetSocket();
                break;
            }
        }
    } // Unlock clients_mtx_

    if (sock < 0) {
        console_.Log(std::format("client ID={} does not connect", id));
        return;
    }

    size_t size = msg.size() + 1;
    send(sock, &size, sizeof(size_t), 0);
    send(sock, msg.c_str(), size, 0);
    console_.Log(std::format("Msg[{}] send to {}", msg, id));
}
