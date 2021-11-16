#pragma once

#ifdef _WIN32
#define _WIN32_WINDOWS
#endif

#define ASIO_STANDALONE
#include <asio.hpp>

bool stoip(const std::string& str, asio::ip::address& ipaddr);

bool getIpv4Address(std::vector<asio::ip::address>& addrList);