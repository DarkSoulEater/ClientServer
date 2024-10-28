#include "Client.hpp"
#include "ParseArg.hpp"

int main(const int argc, const char* argv[]) {
    auto args = ParseArgs(argc, argv);
    Client client(args.proto, args.port);
    client.Start();
    return 0;
}