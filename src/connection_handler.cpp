#include "connection_handler.h"
#include "base_application.h"

using asio::ip::tcp;
using namespace Protocol;

/*-------------------------------
        Connection Handler
---------------------------------*/

ConnectionHandle::ConnectionHandle(tcp::socket soc): socket(std::move(soc)), localhandle(nullptr), app(nullptr), running(true) {
}

ConnectionHandle::~ConnectionHandle() {
    if(localhandle != nullptr){
        if(localhandle->joinable()){
            localhandle->detach(); // break free to allow time to close asynchronously
        }
        delete localhandle;
        stop();
    }
}

void ConnectionHandle::start(Application* application) {
    app = application;
    localhandle = new std::thread(_Handle, this);
}

void ConnectionHandle::stop() {
    running = false;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if(mtxClosed.try_lock()){ // if lock is allowed, thread is still running
            mtxClosed.unlock();
        } else {
            break;
        }
    } while(1);
}

void ConnectionHandle::_Handle() {
    if(app == nullptr) return;

    if(!app->Init()){
        running = false;
    }

    while(running){
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if(!app->Handle()){
            running = false;
        }
    }

    app->Close();

    delete app;
    app = nullptr;

    mtxClosed.lock(); // lock the thread is closed mutex
}

bool ConnectionHandle::readMessage(Message& msg) { // receives message - moves message from queue into return reference
    std::scoped_lock lock(ctxLock);

    if(inbox.empty()) return false;

    msg = std::move(inbox.front());
    inbox.pop();

    return true;
}

bool ConnectionHandle::sendMessage(Message&& msg) { // sends message - given message is moved
    std::scoped_lock lock(ctxLock);

    outbox.emplace(std::move(msg));
    return true;
}

std::string ConnectionHandle::getIPAddress() {
    return socket.remote_endpoint().address().to_string();
}




// Setup Asynchronous Handles For Client


bool ConnectionHandle::asioReadMessageHandle() {
    try {
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

        socket.async_read_some(asio::buffer(buf.ptr, buf.len), std::bind([&](asio::system_error err, size_t read, std::shared_ptr<ConnectionHandle> self){
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

bool ConnectionHandle::asioSendMessageHandle() {
    try {
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

        socket.async_write_some(asio::buffer(buf.ptr, buf.len), std::bind([&](asio::system_error err, size_t written, std::shared_ptr<ConnectionHandle> self){
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
