//Client
#include "client.h"

#include <iostream>
#include <string>

void Client::Handler(std::string msg) {
    std::cout << msg << std::endl;
}

int Client::Run() {
    std::string ipAddr;
    std::cout << "Connect To: ";
    std::cin >> ipAddr;

    asio::io_context service;
    asio::ip::tcp::endpoint ep(asio::ip::address().from_string(ipAddr), PORT);
    asio::ip::tcp::socket socket(service);


    Message msg;
    std::cout << "Connecting to " << ipAddr << "...\n";
    socket.connect(ep);
    std::cout << "Connected!\n";
    
    socket.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{3000}); // receive timeout 3 seconds

    if(!readMessage(socket, msg)){
        std::cout << "failed to read message\n";
    } else {
        std::cout << msg.bytes << "\n";
    }

    pause();
    return 0;
}