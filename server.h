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
    asio::io_service service;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acceptor;

    std::thread* handle;
    std::atomic<bool> serverClose;

    Server();
    virtual ~Server();

    void start();
    void getIpAddress();

    bool Accept(asio::ip::tcp::socket& soc, size_t timeout);
    void Acceptor(asio::ip::tcp::socket* soc);

    void Handler();


    int Run();
    void test();

};