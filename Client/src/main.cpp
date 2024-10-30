#include "Client.hpp"
#include "ParseArg.hpp"

int main(const int argc, const char* argv[]) {
    auto args = ParseArgs(argc, argv, Device::Client);
    Client client(args.proto, args.port, args.ip);
    client.Start();
    return 0;
}