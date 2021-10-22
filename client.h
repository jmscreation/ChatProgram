#pragma once

#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

#include "proto.h"
#include "pause.h"

class Client {
    asio::io_context& context;
public:

    Client(asio::io_context& ctx);

    bool readMessage(asio::ip::tcp::socket& soc, Protocol::Message& msg);
    bool sendMessage(asio::ip::tcp::socket& soc, const Protocol::Message& msg);

    int Run();

    asio::ip::address getIpAddress(const std::string& hostname);
};