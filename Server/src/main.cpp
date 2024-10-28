#include "Server.hpp"
#include "ParseArg.hpp"

int main(const int argc, const char* argv[]) {
    auto args = ParseArgs(argc, argv);
    Server server(args.proto, args.port);
    server.Start();
    return 0;
}