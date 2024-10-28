#pragma once
#include "Config.hpp"
#include "cstring"
#include <iostream>

struct Args {
    Proto proto;
    Port port;
};

Args ParseArgs(const int argc, const char* arg[]);