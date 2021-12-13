#include "application.h"

using namespace Protocol;

std::map<size_t, std::queue<Message>> broadcast;
std::map<size_t, std::queue<Message>> msgqueue;

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

    std::cout << "Press CTRL + C to close server\n";
}

bool ServerApplication::Init() { // init once per client connect
    if(!connection->sendMessage(Message("Welcome to the chat server:"))){
        std::cout << "Failed to send message to new client\n";
        return false;
    } else {
        std::cout << "New client connected [" << connection->getUUID() << "] and added to session\n";
    }
    
    {
        std::scoped_lock lock(bMtx);
        broadcast.insert_or_assign(connection->getUUID(), std::queue<Message>());
        msgqueue.insert_or_assign(connection->getUUID(), std::queue<Message>());
    }
    return true;
}

bool ServerApplication::Handle() { // loop per client
    Message msg;
    if(connection->readMessage(msg)){
        if(msg.header.id == 0){
            std::cout << "incoming message: " << msg.header.length << " bytes\n";
        }
        {
            std::scoped_lock lock(bMtx);
            broadcast[connection->getUUID()].emplace(std::move(msg));
        }
    }
    {
        std::scoped_lock lock(bMtx);
        if(msgqueue[connection->getUUID()].size()){
            connection->sendMessage(std::move(msgqueue[connection->getUUID()].front()));
            msgqueue[connection->getUUID()].pop();
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

bool ServerApplication::StaticHandle(size_t count) {
    std::scoped_lock lock(bMtx);
    for(auto it = broadcast.begin(); it != broadcast.end(); ++it){
        if(it->second.size()){
            Message msg = std::move(it->second.front());
            it->second.pop();
            for(auto _it = msgqueue.begin(); _it != msgqueue.end(); ++_it){
                if(_it->first == it->first) continue; // skip same client
                _it->second.emplace(Message(msg)); // copy
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    return true;
}

void ServerApplication::Close() { // on client disconnect
    std::cout << "Client left: " << connection->getIPAddress() << "\n";
    {
        std::scoped_lock lock(bMtx);
        broadcast.erase(connection->getUUID());
        msgqueue.erase(connection->getUUID());
    }
}



static std::atomic<Window*> window = nullptr;
static Message output(99), input(99);
static std::mutex readMtx;
static std::atomic<bool> connected = false;

void ClientApplication::StaticInit() {
    Window* win = new Window(&ClientApplication::WindowUpdate,
                        [](Window* win) -> bool { return true; }, 300, 400, 1, 1);
    window = win;
    win->Start();
    window = nullptr;
    delete win;
}

bool ClientApplication::Init() {
    if(!connection->sendMessage(Message("New member connected"))){
        std::cout << "Failed to send message\n";
    } else {
        std::cout << "Message sent\n";
        connected = true;
    }

    return true;
}

bool ClientApplication::Handle() {
    
    {
        std::scoped_lock lock(readMtx);
        Message msg;
        if(input.header.id == 99 && connection->readMessage(msg)){
            if(msg.header.id == 0) input = std::move(msg);
        }
        if(output.header.id != 99){
            if(!connection->sendMessage(std::move(output))){
                std::cout << ">> Failed to send message\n";
            }
            output.header.id = 99; // invalid
        }
    }

    if(keepAlive.getMilliseconds() > 1000){
        if(!connection->sendMessage(Message(1))){ // keep-alive
            return false;
        }
        keepAlive.restart();
    }

    if(window == nullptr) return false;

    return true;
}

bool ClientApplication::WindowUpdate(Window* win, float delta) {
    static std::string chat, data;
    static bool terminated = false;

    std::this_thread::sleep_for(std::chrono::milliseconds(8));
    if(connected){
        if(input.header.id != 99){
            std::scoped_lock lock(readMtx);
            Message read(std::move(input));
            input.header.id = 99;
            chat += ">>> ";
            chat.append(read.bytes, read.header.length);
            chat += "\n";
        }
            
        for(int letter = olc::A; letter <= olc::Key::Z; letter++){
            if(win->GetKey(olc::Key(letter)).bPressed){
                data += std::string(1, 'a' + letter - 1 - (32 * win->GetKey(olc::SHIFT).bHeld));
            }
        }

        for(int digit = olc::K0; digit <= olc::Key::K9; digit++){
            if(win->GetKey(olc::Key(digit)).bPressed){
                data += std::string(1, '0' + digit - 27);
            }
        }

        if(win->GetKey(olc::SPACE).bPressed){
            if(data.size()) data += " ";
        }

        if(win->GetKey(olc::BACK).bPressed){
            if(data.size()) data.pop_back();
        }

        if(win->GetKey(olc::ENTER).bPressed){
            std::scoped_lock lock(readMtx);
            chat += "<<< " + data + "\n";
            output = Message(data);
            data.clear();
        }
    } else if(!terminated) {
        chat += "-- Connection Closed --\n";
        terminated = true;
    }

    if(win->GetKey(olc::ESCAPE).bPressed) return false; // close
    
    win->Clear(olc::BLANK);
    win->DrawString({16, 16}, chat);
    win->DrawString({32, win->ScreenHeight() - 24}, data);
    return true;
}

void ClientApplication::Close() {
    std::cout << "Connection Terminated\n";
    connected = false;
}