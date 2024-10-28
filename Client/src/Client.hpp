#pragma once
#include "Config.hpp"
#include "Console.hpp"
#include "DataBuffer.hpp"

class Client {
private:
    Socket clinent_sock_;
    Proto proto_;
    Port port_;

    enum class Status {
        Up,
        Close
    } status_ = Status::Close;
    std::mutex status_mtx_;

    Status GetStatus();
    void SetStatus(Status status);


    void WaitingDataLoop();

    Console console_;
    Commands commands_;
    
    void ConsoleLoop();
    void CommandLoop();

    std::unique_ptr<DataBuffer> LoadData();
    
public:
    Client(Proto proto, Port port) : proto_(proto), port_(port) {}
    ~Client() {
        if (clinent_sock_ >= 0) {
            close(clinent_sock_);
        }
    }

    int Start();
    void Stop();

    void Send(const std::string& msg);
};