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

    inline Application(std::shared_ptr<ConnectionHandle> connection): connection(connection) {}
    virtual ~Application() = default;

    virtual void StaticInit() = 0;  // called once when the application starts - not when a client joins - do not use connection as this is treated as a static method
    virtual bool StaticHandle(size_t clientCount=0) { return true; }; // called within a loop right after application starts - not when a client joins - do not use connection as this runs with no valid connection

    virtual bool Init() = 0;        // called once when the connection is made - for each connection
    virtual bool Handle() = 0;      // called within a loop after the connection is connected - first call is after Init
    virtual void Close() = 0;       // called when the application is closed by the connection - for each connection that leaves
};