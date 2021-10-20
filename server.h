#pragma once


#define _WIN32_WINDOWS
#include <asio.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

#include "proto.h"
#include "pause.h"
#include "clock.h"

class Server {
public:
    asio::io_context& context;
    asio::ip::tcp::acceptor acceptor;

    std::thread* handle;
    std::atomic<bool> serverClose;

    Server(asio::io_context& ctx);
    virtual ~Server();

    void start();
    void getIpAddress();

    void ClientRun(asio::ip::tcp::socket& soc);
    void Acceptor(const asio::error_code& error, asio::ip::tcp::socket* soc);

    void Handler();


    int Run();
    void test();

};