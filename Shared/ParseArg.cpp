#include "ParseArg.hpp"
#include <optional>

std::optional<in_addr_t> ParseIP(const char* addr) {
    std::cerr << "OK\n";
    unsigned char val[4];
    int cnt = sscanf(addr, "%hhu.%hhu.%hhu.%hhu", val, val + 1, val + 2, val + 3);
    if (cnt != 4) 
        return std::optional<in_addr_t>();

    return std::optional<in_addr_t>((val[0] << 24) + (val[1] << 16) + (val[2] << 8) + val[3]);
}

Args ParseArgs(const int argc, const char* arg[], Device device) {
    Proto proto = Proto::TCP;
    Port port = kDefPort;
    in_addr_t ip = INADDR_LOOPBACK;
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
        } else if (device == Device::Client && strcmp(arg[k], "--ip") == 0) {
            if (++k >= argc) {
                std::cout << "Error: IP not set. --help for more information\n";
                exit(1);
            }
            auto ip_res = ParseIP(arg[k]);
            if (!ip_res.has_value()) {
                std::cout << "Error: Unccorect IP addr " << arg[k] << "\n";
                exit(1);
            } else {
                ip = ip_res.value();
                printf("IPa: %08x\n", ip); 
            }
        } else if (strcmp(arg[k], "--help") == 0
                || strcmp(arg[k], "-h")     == 0) {
            std::cout << "Usage " << arg[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "\t-p <Port>\t\t Set port\n";
            std::cout << "\t-P <TCP/UDP>\t\t Set protocol\n";
            if (device == Device::Client) {
                std::cout << "\t--ip <IP>\t\t Set Server ip address\n";
            }
            std::cout << "\t-h, --help\t\t Display help\n";
            exit(0);
        } else {
            std::cout << "Error: Unknow argument " << arg[k] << "\n";
            exit(1);
        }
    }

    return Args({proto, port, ip});
}
