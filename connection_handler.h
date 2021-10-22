#pragma once

#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

#include <string>
#include <queue>


#include "proto.h"


class Application; // forward declaration for Application class

/*
    This class handles an individual client on the server
*/
class ConnectionHandle : public std::enable_shared_from_this<ConnectionHandle> {
    asio::ip::tcp::socket socket;
    std::mutex ctxLock;
    std::queue<Protocol::Message> inbox, outbox;

    std::thread* localhandle;
    Application* app;

    std::atomic<bool> running;
    void _Handle();
    
    Protocol::MessageCache curIn, curOut; // cache messages for incoming and outgoing messages

    bool asioReadMessageHandle(); // init async read handle
    bool asioSendMessageHandle(); // init async write handle
    void start(Application* application); // begin thread localhandle application


public:
    bool readMessage(Protocol::Message& msg);
    bool sendMessage(Protocol::Message&& msg);
    std::string getIPAddress();

    ConnectionHandle(asio::ip::tcp::socket soc);
    virtual ~ConnectionHandle();


    friend class Server;
    friend class Client;
};