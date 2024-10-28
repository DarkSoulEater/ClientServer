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

    void HandlingAcceptLoop();
    void WaitingDataLoop();

    void Disconnect(ClientList::iterator it);


    Console console_;
    Commands commands_;

    void ConsoleLoop();
    void CommandLoop();

    void PrintClients();
    void PrintHistory();
public:
    Server(Proto proto, Port port) : proto_(proto), port_(port) {}
    ~Server() {
        if (serv_socket_ >= 0) {
            close(serv_socket_);
        }
    }

    int Start();
    void Stop();

    void DisconnectAll();

    void SendTo(const std::string& msg, ID id);
};
