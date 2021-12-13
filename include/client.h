#pragma once

#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

#include "connection_handler.h"

#include "base_application.h"

#include "proto.h"
#include "pause.h"
#include "ip_tools.h"

class Client {
    asio::io_context& context;
    std::thread* handle;
    std::shared_ptr<ConnectionHandle> connectionHandle;
    asio::io_context::work idleWork;

public:

    Client(asio::io_context& ctx);
    virtual ~Client();

    template<class T>
    void GenerateClient(asio::ip::tcp::socket soc) {
        static_assert(std::derived_from<T, Application> == true); // generate Application only

        auto addr = soc.remote_endpoint();
        
        std::cout << "Connected to "
                << addr.address().to_string() << ":" << addr.port() << "\n";

        auto client = std::make_shared<ConnectionHandle>(std::move(soc));

        client->start(new T(client)); // instantiate application in memory

        if(!client->asioSendMessageHandle() ||
           !client->asioReadMessageHandle()){
            client->stop();
            client.reset();
            return;
        }

        connectionHandle = client;
    }

    template<class T>
    int Run(const std::string& ipaddr) {
        asio::ip::tcp::endpoint ep;

        try {
            asio::ip::address addr;
            if(stoip(ipaddr, addr)){
                ep = asio::ip::tcp::endpoint(addr, PORT);
            } else {
                std::cout << "IP Address Invalid\n";
                return 1;
            }

            if(ep.address().is_unspecified()){
                std::cout << "IP Address Invalid\n";
                return 1;
            }
        } catch(std::system_error err){
            std::cout << err.what() << "\n";
            return 1;
        }

        try {

            asio::ip::tcp::socket socket(context);
            socket.connect(ep);
            socket.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_SNDTIMEO>{3000}); // send timeout 3 seconds

            std::thread appHandle([](){
                T staticApp(nullptr);
                staticApp.StaticInit();
                while(staticApp.StaticHandle(0)) std::this_thread::sleep_for(std::chrono::milliseconds(20));
            });

            GenerateClient<T>(std::move(socket));

            handle = new std::thread([&](){
                context.run();
            });

            appHandle.join(); // client closes via static application

            if(connectionHandle != nullptr){
                connectionHandle->stop();
                connectionHandle.reset();
            }
            
            context.stop();
            if(handle->joinable()) handle->join();

        } catch(std::system_error err){
            std::cout << err.what() << "\n";
            return 1;
        }

        return 0;
    }
};