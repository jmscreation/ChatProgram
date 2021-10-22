#pragma once

#include <memory>
#include "connection_handler.h"

/*
    Application class is derived for user to make custom application
    This is the application endpoint
*/
class Application {
public:
    std::shared_ptr<ConnectionHandle> connection;

    Application(std::shared_ptr<ConnectionHandle> connection): connection(connection) {}
    virtual ~Application() = default;

    virtual bool Init() = 0;        // called once when the connection is made - for each connection
    virtual bool Handle() = 0;      // called within a loop after the connection is connected - first call is after Init
    virtual void Close() = 0;       // called when the application is closed by the connection - for each connection that leaves
};