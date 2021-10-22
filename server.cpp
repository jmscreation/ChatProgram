#include "server.h"

using asio::ip::tcp;
using namespace Protocol;


/*-------------------------------
        Base Server Class
---------------------------------*/

Server::Server(asio::io_context& ctx):
        context(ctx),
        acceptor(tcp::acceptor(context, tcp::endpoint(tcp::v4(), PORT))),
        handle(nullptr), serverClose(false),
        idleWork(asio::io_context::work(ctx)) {}

Server::~Server() {
    if(handle != nullptr) delete handle;
}

void Server::getIpAddress() {
    try {
        tcp::resolver resolver(context);
        tcp::resolver::query query(asio::ip::host_name(), "", tcp::resolver::query::address_configured);
        tcp::resolver::iterator iter = resolver.resolve(query);
        tcp::resolver::iterator end; // End marker.
        while (iter != end){
            const tcp::endpoint& ep = *iter++;
            if(ep.protocol() == asio::ip::tcp::v4()){
                std::cout << ep.address().to_string() << ":" << short(PORT) << std::endl;
            }
        }
    } catch(std::exception err){
        std::cout << "Failed to get IP Address: " << err.what() << "\n";
    }
}