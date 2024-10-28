#include "Client.hpp"
#include "ParseArg.hpp"
#include "UDP.hpp"

int main(const int argc, const char* argv[]) {
    auto args = ParseArgs(argc, argv);
    // auto sock = udp::Socket();
    // auto connect_res = udp::Connect(sock, args.port);
    // if (connect_res < 0) {
    //     perror("Connect");
    //     return 1;
    // }
    // char buff[] = "Hello world";
    // auto res = udp::Send(sock, buff, sizeof(buff), 0);
    // close(sock);
    // return 0;
    Client client(args.proto, args.port);
    client.Start();
    return 0;
}