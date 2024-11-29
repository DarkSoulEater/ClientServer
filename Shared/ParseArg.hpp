#pragma once
#include "Config.hpp"
#include "cstring"
#include <iostream>

struct Args {
    Proto proto;
    Port port;
    in_addr_t ip;
    const char* crt_path;
    const char* key_path;
};

Args ParseArgs(const int argc, const char* arg[], Device device = Device::Server);