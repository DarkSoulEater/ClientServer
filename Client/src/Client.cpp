#include "Client.hpp"
#include <format>
#include "TCP.hpp"
#include "UDP.hpp"

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
        auto data_ = LoadData(); // Safe unique ptr
        auto& data = *data_.get();
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
                std::thread([this, &cmd]{Send(cmd.str);}).detach();
            } break;

            default:
                console_.Print("[Err]: nknow command type");
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

std::unique_ptr<DataBuffer> Client::LoadData() {
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
    tcp::Recv(clinent_sock_, data.get()->Buffer(), data.get()->Size(), 0);
    return data;
}

int Client::Start() {
    clinent_sock_ = tcp::Socket();
    if (clinent_sock_ < 0) {
        perror("TCP Socket");
        return -1;
    }

    int connect_st = tcp::Connect(clinent_sock_, port_);
    if (connect_st < 0) {
        perror("TCP Connect");
        return -1;
    }

    SetStatus(Status::Up);

    auto data_waiter_thread_ = std::thread([this]{WaitingDataLoop();});
    auto console_loop_       = std::thread([this]{ConsoleLoop();});
    auto command_loop_       = std::thread([this]{CommandLoop();});

    data_waiter_thread_.join();
    console_loop_.join();
    command_loop_.join();

    return 0;
}

void Client::Stop() {
    exit(0);
}

void Client::Send(const std::string &msg) {
    size_t size = msg.size() + 1;
    send(clinent_sock_, &size, sizeof(size_t), 0);
    send(clinent_sock_, msg.c_str(), size, 0);
    console_.Log(std::format("Msg[{}] send to server", msg));
}
