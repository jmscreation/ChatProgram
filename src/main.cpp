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

        Client client(context);
        std::string ip = "localhost";
        if(argc > 2){
            ip = argv[2];
        }

        return client.Run<ClientApplication>(ip);
    }

    return 0;
}