#include "application.h"

using namespace Protocol;

bool ServerApplication::Init() {

    if(!sendMessage(Message("WELCOME HERE"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
    }

    return true;
}

bool ServerApplication::Handle() {
    Message msg;
    if(readMessage(msg)){
        std::cout << "Client says:" << msg.bytes << "\n";
    }

    return true;
}