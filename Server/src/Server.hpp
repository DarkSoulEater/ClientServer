#pragma once
#include <mutex>
#include "Config.hpp"
#include "Client.hpp"
#include "TCP.hpp"
#include "UDP.hpp"
#include "Console.hpp"
#include <openssl/ssl.h>
#include "TLS.hpp"

//
#include <list>
#include <vector>
#include <memory>
typedef std::list<std::unique_ptr<Client>> ClientList;
typedef std::vector<std::unique_ptr<Client>> ClientVec;

// 

class Server {
    Socket serv_socket_;
    Proto proto_;
    Port port_;
    const char* crt_path_;
    const char* key_path_;

    enum class Status {
        Up,
        Close
    } status_ = Status::Close;
    std::mutex status_mtx_;

    Status GetStatus();
    void SetStatus(Status status);

    // For TLS
    bool support_tls_ = false;
    SSL_CTX* ssl_ctx_ = nullptr;
    bool InitTLS();

    std::mutex clients_mtx_;
    ClientList clients_;

    std::mutex clients_history_mtx_;
    ClientVec clients_history_;

    std::mutex time_tmx_;
    Time time_ = 0;
    const Time kLiveTime = 200;

    void IncTime();
    Time GetTime();

    Client* FindClientByID(ID id);
    Client* FindHistoryClientByID(ID id);
    Client* FindClientByPort(Port port);
    Client* FindHistoryClientByPort(Port port);

    int TCPInit();
    int UDPInit();

    void TCPSendTo(const std::string& msg, ID id, bool need_encode = true);
    void UDPSendTO(const std::string& msg, ID id);

    void TCPHandlingAcceptLoop();
    void TCPWaitingDataLoop();
    void UDPWaitingDataLoop();
    std::unique_ptr<DataBuffer> UDPLoadData(sockaddr* sockaddr, socklen_t* socklen);

    void Disconnect(ClientList::iterator it);

    void CheckTimeOut();
    void Forget(ID id); 
    void Remember(ID id);



    Console console_;
    Commands commands_;

    void ConsoleLoop();
    void CommandLoop();

    void PrintClients();
    void PrintHistory();
    void PrintMsgHistory(ID id);
public:
    Server(Proto proto, Port port, const char* crt_path = nullptr, const char* key_path = nullptr) 
        : proto_(proto), port_(port), crt_path_(crt_path), key_path_(key_path) {}
    ~Server() {
        if (serv_socket_ >= 0) {
            close(serv_socket_);
        }
    }

    int Start();
    void Stop();

    void SendTo(const std::string& msg, ID id);
};
