#include "Server.hpp"
#include "ParseArg.hpp"
#include <string>

int main(const int argc, const char* argv[]) {
    auto args = ParseArgs(argc, argv);
    // auto sock = udp::Socket();
    // if (sock < 0) {
    //     perror("Socket:");
    //     exit(1);
    // }
    // auto bind_st = udp::Bind(sock, args.port);
    // if (bind_st < 0) {
    //     perror("Bind:");
    //     exit(1);
    // }
    // sockaddr_in addr;
    // char* buff[1024] = {0};
    // for (;;) {
    //     auto res = udp::Recv(sock, buff, sizeof(buff), 0);
    //     std::cout << std::string((char*)buff);
    // }
    // close(sock);
    // return 0;
    Server server(args.proto, args.port);
    server.Start();
    return 0;
}