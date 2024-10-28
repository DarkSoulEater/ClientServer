#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef in_port_t Port;
typedef int Socket;

enum class Proto {
    TCP,
    UDP
};

enum class Device {
    Server,
    Client
};

const int kDefPort = 10010;
