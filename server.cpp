//server

#include "server.h"

// server

using asio::ip::tcp;

Server::Server(asio::io_context& ctx):
        context(ctx),
        acceptor(tcp::acceptor(context, tcp::endpoint(tcp::v4(), PORT))),
        handle(nullptr), serverClose(false) {}

Server::~Server() {
    if(handle != nullptr) delete handle;
    //if(acceptor != nullptr) delete acceptor;
}



void Server::ClientRun(tcp::socket& soc) {

    auto addr = soc.remote_endpoint();
    
    std::cout << "someone connected from: "
        << addr.address().to_string() << ":" << addr.port() << std::endl;

    Message msg("WELCOME HERE");
    
    if(!sendMessage(soc, msg)){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    soc.close();
}


void Server::Handler() {
    
    acceptor.async_accept([this](std::error_code er, tcp::socket soc){
        if(!er){
            if(soc.is_open()){
                soc.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_SNDTIMEO>{3000}); // send timeout 3 seconds
                std::cout << "new connection\n";
                ClientRun(soc);
            }
        } else {
            std::cout << "Error when accepting connection: " << er << "\n";
        }
        Handler();
    });
}

void Server::start() {
    std::cout << "listening on: " << std::endl;

    handle = new std::thread([&](){
        Handler();

        context.run();
    });
}


int Server::Run() {
    start();

    pause();

    std::cout << "server close...\n";
    context.stop();
    handle->join();

    return 0;
}

void Server::test() {
    
}

void Server::getIpAddress() {
    tcp::resolver resolver(context);
    tcp::resolver::query query(asio::ip::host_name(), "");
    tcp::resolver::iterator iter = resolver.resolve(query);
    tcp::resolver::iterator end; // End marker.
    while (iter != end){
        tcp::endpoint ep = *iter++;
        std::cout << ep << std::endl;
    }
}