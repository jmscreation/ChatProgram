#pragma once

#include "clock.h"
#include "server.h"

class ServerApplication : public Application {
    Clock keepAlive;
public:
    ServerApplication(std::shared_ptr<ClientHandle> client): Application(client) {}

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;
};