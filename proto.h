#pragma once

#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

#include <cstdlib>
#include <string>
#include <iostream>

#define PORT 6000

namespace Protocol {
    struct Message {
        struct Header {
            uint32_t id;
            uint32_t length;
        };

        Header header;

        const char* bytes;

        inline Message& operator=(const std::string& data) {
            message = data;
            header.length = message.size();
            bytes = message.data();

            return *this;
        }

        inline Message& operator%(uint32_t id) {
            header.id = id;

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

        Message(): header({0,0}) {}
        Message(const std::string& msg) { *(this) = msg; header.id = 0; }
        Message(uint32_t id, const std::string& msg) { *(this) = msg; header.id = id; }

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

}