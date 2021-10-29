#pragma once

#include "base_application.h"
#include "proto.h"
#include "clock.h"
#include "ip_tools.h"

class ServerApplication : public Application {
    Clock keepAlive;
public:
    ServerApplication(std::shared_ptr<ConnectionHandle> connection): Application(connection) {}

    virtual void StaticInit() override;

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;
};


class ClientApplication : public Application {
    Clock keepAlive, sendClock;
public:
    ClientApplication(std::shared_ptr<ConnectionHandle> connection): Application(connection) {}

    virtual void StaticInit() override;

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;
};