//Client
#include "client.h"

#include <iostream>
#include <string>

using asio::ip::tcp;
using namespace Protocol;

Client::Client(asio::io_context& ctx):
    context(ctx),
    handle(nullptr),
    connectionHandle(nullptr),
    idleWork(asio::io_context::work(ctx)) {}



Client::~Client() {
    if(handle != nullptr) delete handle;
}