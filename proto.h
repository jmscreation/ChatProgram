#pragma once

#define _WIN32_WINDOWS
#include <asio.hpp>

#include <cstdlib>
#include <string>
#include <iostream>

#define PORT 6000

namespace Protocol {
    struct Message {
        struct Header {
            size_t id;
            size_t length;
        };

        Header header;

        const char* bytes;

        inline Message& operator=(const std::string& data) {
            message = data;
            header.id = 0;
            header.length = message.size();
            bytes = message.data();

            return *this;
        }

        inline Message& operator=(Message&& msg) {
            message = std::move(msg.message);
            header.id = msg.header.id;
            header.length = message.size();
            bytes = message.data();

            return *this;
        }

        Message(Message&& msg) { // move constructor emulates move assignment
            *this = std::move(msg);
        }

        Message() = default;
        Message(const std::string& msg) { *(this) = msg; }

    private:
        std::string message;
    };
    struct MessageCache {
        // main cache data
        Message msg;
        size_t pos, len;

        // for reading data
        std::array<char, 1024> cache;
        std::string readbuf;

        // current cache status
        enum Status {
            EMPTY, PENDING, PENDING_HEADER, READY
        };
        Status status;

        MessageCache(): msg(""), pos(0), status(EMPTY) {}
    };

    extern bool asioSendMessage(asio::ip::tcp::socket& soc, const Message& msg);
    extern bool asioReadMessage(asio::ip::tcp::socket& soc, Message& msg);

}