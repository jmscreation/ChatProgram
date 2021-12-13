#pragma once

#include "base_application.h"
#include "proto.h"
#include "clock.h"
#include "ip_tools.h"
#include "window.h"

#include <mutex>
#include <map>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <functional>

class ServerApplication : public Application {
    Clock keepAlive;
    std::mutex bMtx;
public:
    ServerApplication(std::shared_ptr<ConnectionHandle> connection): Application(connection) {}

    virtual void StaticInit() override;
    virtual bool StaticHandle(size_t count) override;

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;
};


class ClientApplication : public Application {
    Clock keepAlive;
public:
    ClientApplication(std::shared_ptr<ConnectionHandle> connection): Application(connection) {}

    virtual void StaticInit() override;
    inline virtual bool StaticHandle(size_t count) override { return false; } // do not use - close app

    virtual bool Init() override;
    virtual bool Handle() override;
    virtual void Close() override;

    static bool WindowUpdate(Window* win, float delta);
};