#pragma once
#include "Config.hpp"
#include "Console.hpp"
#include "DataBuffer.hpp"
#include "tls/Endpoint.hpp"
#include "tls/TLS.hpp"

class Client {
private:
    Socket clinent_sock_;
    Proto proto_;
    Port port_;
    in_addr_t serv_ip_;

    enum class Status {
        Up,
        Close
    } status_ = Status::Close;
    std::mutex status_mtx_;

    
    int TCPInit();
    int UDPInit();

    Status GetStatus();
    void SetStatus(Status status);


    void WaitingDataLoop();

    Console console_;
    Commands commands_;
    
    void ConsoleLoop();
    void CommandLoop();

    std::unique_ptr<DataBuffer> TCPLoadData();
    std::unique_ptr<DataBuffer> UDPLoadData();

    bool need_tls_ = true;
    bool under_tls_ = false;

    bool InitTLS();
    EncryptPoint* encr_point_ = nullptr;
    std::unique_ptr<TLS> tls_;
    // bool HandShake();
    // DataBuffer TLSGetWantData();
    
public:
    Client(Proto proto, Port port, in_addr_t server_ip) : proto_(proto), port_(port), serv_ip_(server_ip) {}
    ~Client() {
        if (clinent_sock_ >= 0) {
            close(clinent_sock_);
        }
    }

    int Start();
    void Stop();

    void Send(const std::string& msg);
};