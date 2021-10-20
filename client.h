#pragma once

#define _WIN32_WINDOWS
#include <asio.hpp>

#include "proto.h"
#include "pause.h"

class Client {
public:
    void Handler(std::string msg);

    int Run();
};