#include "application.h"

using namespace Protocol;

bool ServerApplication::Init() {

    if(!connection->sendMessage(Message("WELCOME HERE"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    return true;
}

bool ServerApplication::Handle() {
    Message msg;
    if(connection->readMessage(msg)){
        if(msg.header.id == 0){
            std::cout << "Client Message: " << msg.bytes << "\n";
        }
    }

    if(keepAlive.getMilliseconds() > 1000){
        if(!connection->sendMessage(Message(1, "keep-alive"))){
            return false;
        }
        keepAlive.restart();
    }

    return true;
}

void ServerApplication::Close() {
    std::cout << "Client left: " << connection->getIPAddress() << "\n";
}