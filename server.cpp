//server

#include "server.h"

// server

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


/*-------------------------------
        Base Client Handle
---------------------------------*/

ClientHandle::ClientHandle(tcp::socket soc): socket(std::move(soc)), localhandle(nullptr), app(nullptr), running(true) {
}

ClientHandle::~ClientHandle() {
    if(localhandle != nullptr){
        running = false;
        if(localhandle->joinable()){
            localhandle->detach(); // break free to allow time to close asynchronously
        }
        delete localhandle;
    }
}

void ClientHandle::start(Application* application) {
    app = application;
    localhandle = new std::thread(_Handle, this);
}

void ClientHandle::_Handle() {
    if(app == nullptr) return;

    if(!app->Init()){
        running = false;
    }

    while(running){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if(!app->Handle()){
            running = false;
        }
    }

    app->Close();

    delete app;
    app = nullptr;
}

bool ClientHandle::readMessage(Message& msg) { // receives message - moves message from queue into return reference
    std::scoped_lock lock(ctxLock);

    if(inbox.empty()) return false;

    msg = std::move(inbox.front());
    inbox.pop();

    return true;
}

bool ClientHandle::sendMessage(Message&& msg) { // sends message - given message is moved
    std::scoped_lock lock(ctxLock);

    outbox.emplace(std::move(msg));
    return true;
}

std::string ClientHandle::getIPAddress() {
    return socket.remote_endpoint().address().to_string();
}




// Setup Asynchronous Handles For Client


bool ClientHandle::asioReadMessageHandle() {
    try {
        if(!socket.is_open()){ // check if client is still connected
            running = false;
            return false;
        }

        struct Buffer {
            void* ptr;
            size_t len;
        } buf; // create a temporary cache descriptor for asio::buffer

        switch(curIn.status){
            case MessageCache::READY:{
                std::scoped_lock lock(ctxLock);

                inbox.emplace(std::move(curIn.msg)); // move next message to send into cache
                curIn.pos = 0;
                curIn.msg = ""; // generate new cache message
                
                curIn.readbuf.clear(); // empty content buffer
                curIn.status = MessageCache::EMPTY;
            }
            [[fallthrough]];

            case MessageCache::EMPTY:
            case MessageCache::PENDING_HEADER:{
                buf.ptr = reinterpret_cast<void*>( (char*)(&curIn.msg.header) + curIn.pos );
                buf.len = sizeof(curIn.msg.header) - curIn.pos;

                curIn.len = sizeof(curIn.msg.header);
            } break;

            case MessageCache::PENDING:{
                buf.ptr = (void*)curIn.cache.data(); // use cache when writing to content buffer (readbuf)
                buf.len = std::min(curIn.msg.header.length - curIn.pos, curIn.cache.size());
                curIn.len = curIn.msg.header.length;
            } break;
        }

        socket.async_read_some(asio::buffer(buf.ptr, buf.len), std::bind([&](asio::system_error err, size_t read, std::shared_ptr<ClientHandle> self){
            if(!self->running) return;
            
            if(curIn.status == MessageCache::EMPTY && read){
                curIn.status = MessageCache::PENDING_HEADER;
            }
            if(curIn.status != MessageCache::EMPTY){ // currently streaming
                curIn.pos += read;

                if(read && (curIn.status == MessageCache::PENDING)){ // streaming message content
                    curIn.readbuf.append(curIn.cache.data(), read); // append cache to read buffer if reading message content
                }

                if(!read){ // check for problems when streaming
                    if(curIn.pos < curIn.len){
                        std::cout << "Failed to async_read: " << err.what() << "\n";
                        running = false;
                        return;
                    }
                }

                if(curIn.pos >= curIn.len){ // finished streaming message part
                    switch(curIn.status){
                        case MessageCache::PENDING_HEADER:
                            curIn.status = MessageCache::PENDING; // ready to stream message content
                        break;
                        case MessageCache::PENDING:
                            curIn.msg = curIn.readbuf; // copy content buffer into message
                            curIn.status = MessageCache::READY; // ready to append to inbox
                        break;
                    }
                    curIn.pos = 0; // always reset position when finished writing
                }
            }

            self->asioReadMessageHandle();
        }, std::placeholders::_1, std::placeholders::_2, shared_from_this()));

    } catch(std::exception err){
        running = false;
        std::cout << err.what() << "\n";
        return false;
    }

    return true;
}

bool ClientHandle::asioSendMessageHandle() {
    try {
        if(!socket.is_open()){ // check if client is still connected
            running = false;
            return false;
        }

        struct Buffer {
            void* ptr;
            size_t len;
        } buf; // create a temporary cache descriptor for asio::buffer

        switch(curOut.status){
            case MessageCache::EMPTY:{
                std::scoped_lock lock(ctxLock);

                if(!outbox.empty()){
                    curOut.msg = std::move(outbox.front()); // move next message to send into cache
                    outbox.pop();
                    curOut.pos = 0;
                    curOut.status = MessageCache::PENDING_HEADER;
                } else {
                    break;
                }
            }
            [[fallthrough]];
            
            case MessageCache::PENDING_HEADER:{
                buf.ptr = reinterpret_cast<void*>( (char*)(&curOut.msg.header) + curOut.pos );
                buf.len = sizeof(curOut.msg.header) - curOut.pos;
                curOut.len = sizeof(curOut.msg.header);
            } break;

            case MessageCache::PENDING:{
                buf.ptr = (void*)(curOut.msg.bytes + curOut.pos);
                buf.len = curOut.msg.header.length - curOut.pos;
                curOut.len = curOut.msg.header.length;
            } break;
        }

        socket.async_write_some(asio::buffer(buf.ptr, buf.len), std::bind([&](asio::system_error err, size_t written, std::shared_ptr<ClientHandle> self){
            if(!self->running) return;

            if(curOut.status != MessageCache::EMPTY){
                curOut.pos += written;
                if(!written){
                    if(curOut.pos < curOut.len){
                        std::cout << "Failed to async_write: " << err.what() << "\n";
                        self->running = false;
                        return;
                    }
                }
                if(curOut.pos >= curOut.len){
                    switch(curOut.status){
                        case MessageCache::PENDING_HEADER:
                            curOut.status = MessageCache::PENDING;
                        break;
                        case MessageCache::PENDING:
                            curOut.status = MessageCache::EMPTY;
                        break;
                    }
                    curOut.pos = 0; // always reset position when finished writing
                }
            }

            self->asioSendMessageHandle();
        }, std::placeholders::_1, std::placeholders::_2, shared_from_this()));

        
    } catch(std::exception err){
        running = false;
        std::cout << err.what() << "\n";
        return false;
    }

    return true;
}
