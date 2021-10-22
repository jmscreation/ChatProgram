#include "application.h"

using namespace Protocol;

bool ServerApplication::Init() {

    if(!client->sendMessage(Message("WELCOME HERE"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    return true;
}

bool ServerApplication::Handle() {
    Message msg;
    if(client->readMessage(msg)){
        if(msg.header.id == 0){
            std::cout << "Client Message: " << msg.bytes << "\n";
        }
    }

    if(keepAlive.getMilliseconds() > 1000){
        if(!client->sendMessage(Message(1, "keep-alive"))){
            return false;
        }
        keepAlive.restart();
    }

    return true;
}

void ServerApplication::Close() {
    std::cout << "Client left: " << client->getIPAddress() << "\n";
}