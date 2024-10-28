#include "ParseArg.hpp"

Args ParseArgs(const int argc, const char* arg[]) {
    Proto proto = Proto::TCP;
    Port port = kDefPort;
    for (size_t k = 1; k < argc; ++k) {
        if (strcmp(arg[k], "-P") == 0) {
            if (++k >= argc) {
                std::cout << "Error: Protocol not set. --help for more information\n";
                exit(1);
            }
            if (strcmp(arg[k], "TCP") == 0
             || strcmp(arg[k], "tcp") == 0
             || strcmp(arg[k], "Tcp") == 0) {
                proto = Proto::TCP;
            } else if (
                strcmp(arg[k], "UDP") == 0
             || strcmp(arg[k], "udp") == 0
             || strcmp(arg[k], "Udp") == 0
            ) {
                proto = Proto::UDP;
            } else {
                std::cout << "Error: Unknow protocol " << arg[k];
                exit(1);
            }
        } else if (strcmp(arg[k], "-p") == 0) {
            if (++k >= argc) {
                std::cout << "Error: Port not set. --help for more information\n";
                exit(1);
            }
            if (atoi(arg[k]) == 0) {
                std::cout << "Error: Uncorrect port value " << arg[k] << "\n";
                exit(1);
            }
            port = atoi(arg[k]);
        } else if (strcmp(arg[k], "--help") == 0
                || strcmp(arg[k], "-h")     == 0) {
            std::cout << "Usage " << arg[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "\t-p <Port>\t\t Set port\n";
            std::cout << "\t-P <TCP/UDP>\t\t Set protocol\n";
            std::cout << "\t-h, --help\t\t Display help\n";
            exit(0);
        } else {
            std::cout << "Error: Unknow argument " << arg[k] << "\n";
            exit(1);
        }
    }

    return Args({proto, port});
}
