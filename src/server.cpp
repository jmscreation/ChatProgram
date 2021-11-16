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