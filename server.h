#pragma once


#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <queue>
#include <memory>

#include "proto.h"
#include "pause.h"
#include "clock.h"

class ClientHandle; // forward declaration for Application

/*
    Application class is derived for user to make custom application
    This is the application endpoint
*/
class Application {
public:
    std::shared_ptr<ClientHandle> client;

    Application(std::shared_ptr<ClientHandle> client): client(client) {}
    virtual ~Application() = default;

    virtual bool Init() = 0;        // called once when the client is connected - for each client
    virtual bool Handle() = 0;      // called within a loop after the client is connected - first call is after Init
    virtual void Close() = 0;       // called when the application is closed by the client - for each client that leaves
};

/*
    This class handles an individual client on the server
*/
class ClientHandle : public std::enable_shared_from_this<ClientHandle> {
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

    ClientHandle(asio::ip::tcp::socket soc);
    virtual ~ClientHandle();


    friend class Server;
};



class Server {
public:
    asio::io_context& context;
    asio::ip::tcp::acceptor acceptor;

    std::thread* handle;
    std::atomic<bool> serverClose;
    asio::io_context::work idleWork;

    std::mutex joinMtx;
    std::vector<std::shared_ptr<ClientHandle>> clientHandles;

    Server(asio::io_context& ctx);
    virtual ~Server();

    void getIpAddress();

    template<class T>
    void GenerateClient(asio::ip::tcp::socket soc) {
        static_assert(std::derived_from<T, Application> == true); // generate Application only

        std::scoped_lock lock(joinMtx); // don't break system when multiple clients join at the same time

        auto addr = soc.remote_endpoint();
        
        std::cout << "Client connecting from: "
                << addr.address().to_string() << ":" << addr.port() << "\n";

        auto client = std::make_shared<ClientHandle>(std::move(soc));

        client->start(new T(client)); // instantiate application in memory

        if(!client->asioSendMessageHandle() ||
           !client->asioReadMessageHandle()){
            soc.close();
            std::cout << "init async io failed\n";
            return;
        }

        clientHandles.push_back(client);
    }

    template<class T>
    void Handler(){
    
        acceptor.async_accept([this](std::error_code er, asio::ip::tcp::socket soc){
            if(!er){
                if(soc.is_open()){
                    soc.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_SNDTIMEO>{3000}); // send timeout 3 seconds
                    GenerateClient<T>(std::move(soc));
                }
            } else {
                std::cout << "Error when accepting connection: " << er << "\n";
            }
            Handler<T>();
        });
    }

    template<class T>
    int Run() {
        std::cout << "Server listening on:\n";

        handle = new std::thread([&](){
            getIpAddress();

            context.run();
        });

        Handler<T>();
        
        std::atomic<bool> gccRunning = true;
        std::thread garbageClientCollector([&](){
            do {
                std::scoped_lock lock(joinMtx); // don't break system when accessing client table
                for(int i=0; i < clientHandles.size(); ++i){
                    auto client = clientHandles[i];
                    if(!client->running){
                        clientHandles.erase(clientHandles.begin() + i--);
                        continue;
                    }
                }
            } while(gccRunning);
            
            {   // when garbage collector finishes, cleanup all active clients
                std::scoped_lock lock(joinMtx);
                for(auto client : clientHandles){ // stop running all client application handles
                    client->running = false;
                }
                clientHandles.clear(); // free all shared pointers from client table
            }
        });

        std::cout << "Press any key to close server...\n";
        pause();

        std::cout << "Closing Server...\n";
        context.stop();

        gccRunning = false;
        if(garbageClientCollector.joinable()) garbageClientCollector.join();

        if(handle != nullptr && handle->joinable()) handle->join();

        return 0;
    }

};