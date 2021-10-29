#include "server.h"
#include "client.h"
#include "pause.h"

#include "application.h"

#include <iostream>

int main(int argc, char** argv) {

    if(argc > 1){
        std::string v = argv[1];

        asio::io_context context;

        if(v == "server"){
            Server server(context);

            return server.Run<ServerApplication>(); // ServerApplication is what each client connects to
        }

        if(v == "client"){
            Client client(context);

            return client.Run<ClientApplication>("localhost");
        }
    }

    std::cout << "Please specify:\nserver or client\n";

    pause();

    return 0;
}