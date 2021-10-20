//Client
#include "client.h"

#include <iostream>
#include <string>

using asio::ip::tcp;
using namespace Protocol;

Client::Client(asio::io_context& ctx): context(ctx) {} 

asio::ip::address Client::getIpAddress(const std::string& hostname) {
    asio::ip::address addr;
    try {
        addr = asio::ip::address::from_string(hostname);
        return addr;
    } catch(asio::system_error err) {
    }

    try {
        tcp::resolver::query query(tcp::v4(), hostname.data(), "");
        tcp::resolver resolver(context);
        tcp::resolver::iterator iter = resolver.resolve(query);
        tcp::resolver::iterator end; // End marker.
        while (iter != end){
            const tcp::endpoint& ep = *iter++;
            if(ep.protocol() == asio::ip::tcp::v4()) return ep.address();
        }
    } catch(std::system_error err){
        std::cout << "Failed to get IP Address: " << err.what() << "\n";
    }
    return asio::ip::address::from_string("0.0.0.0");
}

int Client::Run() {
    std::string ipAddr;
    std::cout << "Connect To: ";
    std::cin >> ipAddr;
    try {
        tcp::endpoint ep(getIpAddress(ipAddr), PORT);
        if(ep.address().is_unspecified()){
            std::cout << "IP Address Invalid\n";
            return 1;
        }
        tcp::socket socket(context);


        Message msg;
        std::cout << "Connecting to " << ipAddr << "...\n";
        socket.connect(ep);
        socket.set_option(asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>{3000}); // receive timeout 3 seconds

        std::cout << "Connected!\n";
        
        if(!readMessage(socket, msg)){
            std::cout << "failed to read message\n";
        } else {
            std::cout << msg.bytes << "\n";
        }

    } catch(std::system_error err){
        std::cout << err.what() << "\n";
        return 1;
    }
    


    pause();
    return 0;
}


/*
    Temporarily Use The Old Synchronous Send/Receive Functions For Client
*/

bool Client::readMessage(tcp::socket& soc, Message& msg) {
    std::cout << "read message:\n";
    try {
        std::cout << "\tread header\n";
        size_t pos = 0;
        do {
            size_t read = soc.read_some(asio::buffer( ((char*)&msg.header) + pos, sizeof(msg.header) - pos));
            pos += read;

            if(!read){
                if(pos != sizeof(msg.header)) return false;
                break;
            }
        } while(pos < sizeof(msg.header));

        std::cout << "\tread content\n";
        std::string buf;
        pos = 0;
        while(pos < msg.header.length){
            char data[1024]; // 1kb cache
            size_t read = soc.read_some(asio::buffer(data, std::min( sizeof(data), msg.header.length - pos ))); // read into cache
            pos += read;
            buf.append(data, read);
        }

        msg = buf;
    } catch(std::system_error err) {
        std::cout << err.what() << "\n";
        return false;
    }

    return true;
}

bool Client::sendMessage(tcp::socket& soc, const Message& msg) {
    size_t pos = 0;
    while(pos < sizeof(msg.header)){
        size_t written = soc.write_some(asio::buffer(((char*)&msg.header) + pos, sizeof(msg.header) - pos));
        pos += written;
        if(!written){
            if(pos != sizeof(msg.header)) return false;
            break;
        }
    }

    const char *cpos = msg.bytes, *end = cpos + msg.header.length;
    while(cpos < end){
        size_t written = soc.write_some(asio::buffer(cpos, end - cpos));
        cpos += written;
        if(!written){
            if(cpos < end) return false;
            break;
        }
    }

    return true;
}