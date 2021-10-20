#include "proto.h"

bool sendMessage(asio::ip::tcp::socket& soc, const Message& msg) {
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

bool readMessage(asio::ip::tcp::socket& soc, Message& msg) {
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