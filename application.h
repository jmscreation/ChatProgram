#pragma once

#include "base_application.h"
#include "proto.h"
#include "clock.h"

class ServerApplication : public Application {
    Clock keepAlive;
public:
    ServerApplication(std::shared_ptr<ConnectionHandle> connection): Application(connection) {}

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;
};