#include "application.h"

using namespace Protocol;

void ServerApplication::StaticInit() { // init once per server
    std::vector<asio::ip::address> iplist;
    if(getIpv4Address(iplist)){
        std::cout << "Server listening on:\n";
        for(const asio::ip::address& ip : iplist){
            std::cout << "\t" << ip.to_string() << ":" << short(PORT) << "\n";
        }
    } else {
        std::cout << "Failed to resolve local ip addresses!\n";
    }
}

bool ServerApplication::Init() { // init once per client connect

    if(!connection->sendMessage(Message("WELCOME HERE"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    return true;
}

bool ServerApplication::Handle() { // loop per client
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

void ServerApplication::Close() { // on client disconnect
    std::cout << "Client left: " << connection->getIPAddress() << "\n";
}





void ClientApplication::StaticInit() {

}

bool ClientApplication::Init() {

    if(!connection->sendMessage(Message("I am a client!"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    return true;
}

bool ClientApplication::Handle() {
    Message msg;

    if(connection->readMessage(msg)){
        if(msg.header.id == 0){
            std::cout << msg.bytes << "\n";
        }
    }

    if(sendClock.getMilliseconds() > 3000){    
        if(!connection->sendMessage(Message("test message response"))){
            std::cout << "Failed to send message\n";
            return false;
        }
        sendClock.restart();
    }


    if(keepAlive.getMilliseconds() > 1000){
        if(!connection->sendMessage(Message(1, "keep-alive"))){
            return false;
        }
        keepAlive.restart();
    }

    return true;
}

void ClientApplication::Close() {
    std::cout << "Connection Terminated\n";
}