#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef in_port_t Port;
typedef int Socket;
typedef size_t ID;

enum class Proto {
    TCP,
    UDP
};

enum class Device {
    Server,
    Client
};

enum class SocketStatus {
    Connected,
    Disconnected
};

const int kDefPort = 10010;
