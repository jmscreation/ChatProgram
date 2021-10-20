//server

#include "server.h"

// server

using asio::ip::tcp;

Server::Server(): ep(tcp::endpoint(tcp::v4(), PORT)),
        acceptor(tcp::acceptor(service, ep)),
        handle(nullptr), serverClose(false) {}

Server::~Server() {
    if(handle != nullptr) delete handle;
}



bool Server::Accept(tcp::socket& soc, size_t timeout) {

    acceptor.async_accept(soc, std::bind(&Server::Acceptor, this, &soc) );
    
    service.run();

    Clock timer;

    while(timer.getMilliseconds() < timeout && !soc.is_open());

    return soc.is_open();
}

void Server::Acceptor(tcp::socket* soc) {
    std::cout << "new connection\n";
}

void Server::Handler() {
    tcp::socket* soc = nullptr;

    while(!serverClose){
        if(soc == nullptr){
            soc = new tcp::socket(service);
        }
        try {
            if(!Accept(*soc, 500)) continue;
            
            soc->set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{3000}); // receive timeout 3 seconds

            std::cout << "someone connected" << std::endl;

            Message msg("WELCOME HERE");
            
            if(!sendMessage(*soc, msg)){
                std::cout << "Failed to send message\n";
            } else {
                std::cout << "Message sent\n";
            }

            soc->close();
            delete soc;
            soc = nullptr;

        } catch(std::exception e){
            std::cout << e.what() << "\n";
            continue;
        }
    }

    delete soc;
    soc = nullptr;
}

void Server::start() {
    std::cout << "listening on: " << std::endl;

    handle = new std::thread(Handler, this);

}


int Server::Run() {
    start();

    pause();

    std::cout << "server close...\n";
    serverClose = true;

    handle->join();
    return 0;
}

void Server::test() {
    
}

void Server::getIpAddress() {
    tcp::resolver resolver(service);
    tcp::resolver::query query(asio::ip::host_name(), "");
    tcp::resolver::iterator iter = resolver.resolve(query);
    tcp::resolver::iterator end; // End marker.
    while (iter != end){
        tcp::endpoint ep = *iter++;
        std::cout << ep << std::endl;
    }
}