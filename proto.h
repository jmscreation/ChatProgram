#pragma once

#define _WIN32_WINDOWS
#include <asio.hpp>

#include <cstdlib>
#include <string>
#include <iostream>

#define PORT 6000

struct Message {
    struct Header {
        size_t id;
        size_t length;
    };

    Header header;

    const char* bytes;

    inline Message& operator=(const std::string& data){
        message = data;
        header.id = 0;
        header.length = message.size();
        bytes = message.data();

        return *this;
    }

    Message() = default;
    Message(const std::string& msg) { *(this) = msg; }

private:
    std::string message;
};


extern bool sendMessage(asio::ip::tcp::socket& soc, const Message& msg);
extern bool readMessage(asio::ip::tcp::socket& soc, Message& msg);