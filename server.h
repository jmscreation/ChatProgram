#pragma once


#define _WIN32_WINDOWS
#include <asio.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <queue>

#include "proto.h"
#include "pause.h"
#include "clock.h"


class ClientHandle {
    std::mutex ctxLock;
    std::queue<Protocol::Message> inbox, outbox;

    std::thread* localhandle;
    std::atomic<bool> running;
    asio::ip::tcp::socket* socket;
    void _Handle();
    
    Protocol::MessageCache curIn, curOut; // cache messages for incoming and outgoing messages

    bool asioReadMessageHandle(); // init async read handle
    bool asioSendMessageHandle(); // init async write handle

protected:
    bool readMessage(Protocol::Message& msg);
    bool sendMessage(Protocol::Message&& msg);
    void start(asio::ip::tcp::socket& soc); // begin thread localhandle application

public:

    ClientHandle();
    virtual ~ClientHandle();
    
    virtual bool Init() { return true; }      // called once when the client is connected
    virtual bool Handle() { return true; }    // called within a loop after the client is connected - first call is after Init


    friend class Server;
};



class Server {
public:
    asio::io_context& context;
    asio::ip::tcp::acceptor acceptor;

    std::thread* handle;
    std::atomic<bool> serverClose;

    std::vector<ClientHandle*> clientHandles;

    Server(asio::io_context& ctx);
    virtual ~Server();

    void getIpAddress();

    template<class T>
    void GenerateClient(asio::ip::tcp::socket& soc) {

        static_assert(std::derived_from<T, ClientHandle> == true); // generate ClientHandles only

        auto addr = soc.remote_endpoint();
        
        std::cout << "Client connecting from: "
                << addr.address().to_string() << ":" << addr.port() << "\n";

        ClientHandle* client = new T();
        client->socket = &soc;

        if(!client->asioSendMessageHandle() ||
           !client->asioReadMessageHandle()){
            delete client;
            soc.close();
            std::cout << "init async io failed\n";
            return;
        }
        client->start(soc);

        clientHandles.push_back(client);
    }

    template<class T>
    void Handler(){
    
        acceptor.async_accept([this](std::error_code er, asio::ip::tcp::socket soc){
            if(!er){
                if(soc.is_open()){
                    soc.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_SNDTIMEO>{3000}); // send timeout 3 seconds
                    std::cout << "new connection\n";
                    GenerateClient<T>(soc);
                }
            } else {
                std::cout << "Error when accepting connection: " << er << "\n";
            }
            Handler<T>();
        });
    }

    template<class T>
    int Run() {
        std::cout << "listening on: " << std::endl;

        handle = new std::thread([&](){
            Handler<T>();

            getIpAddress();

            context.run();
        });

        pause();

        std::cout << "server close...\n";
        context.stop();
        handle->join();

        return 0;
    }

};