#pragma once
#include <mutex>
#include "Config.hpp"
#include "Client.hpp"
#include "TCP.hpp"
#include "UDP.hpp"
#include "Console.hpp"

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

    enum class Status {
        Up,
        Close
    } status_ = Status::Close;
    std::mutex status_mtx_;

    Status GetStatus();
    void SetStatus(Status status);

    std::mutex clients_mtx_;
    ClientList clients_;

    std::mutex clients_history_mtx_;
    ClientVec clients_history_;

    Client* FindClientByID(ID id);
    Client* FindHistoryClientByID(ID id);
    Client* FindClientByPort(Port port);

    int TCPInit();
    int UDPInit();

    void TCPSendTo(const std::string& msg, ID id);
    void UDPSendTO(const std::string& msg, ID id);

    void TCPHandlingAcceptLoop();
    void TCPWaitingDataLoop();
    void UDPWaitingDataLoop();
    std::unique_ptr<DataBuffer> UDPLoadData(sockaddr* sockaddr, socklen_t* socklen);

    void Disconnect(ClientList::iterator it);


    Console console_;
    Commands commands_;

    void ConsoleLoop();
    void CommandLoop();

    void PrintClients();
    void PrintHistory();
    void PrintMsgHistory(ID id);
public:
    Server(Proto proto, Port port) : proto_(proto), port_(port) {}
    ~Server() {
        if (serv_socket_ >= 0) {
            close(serv_socket_);
        }
    }

    int Start();
    void Stop();

    void SendTo(const std::string& msg, ID id);
};
