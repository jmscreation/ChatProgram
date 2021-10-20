#include "server.h"
#include "client.h"
#include "pause.h"

#include <iostream>

int main(int argc, char** argv) {

    if(argc > 1){
        std::string v = argv[1];

        if(v == "server"){
            Server server;

            return server.Run();
        }

        if(v == "client"){
            Client client;

            return client.Run();
        }
    }

    std::cout << "Please specify:\nserver or client\n";

    pause();

    return 0;
}