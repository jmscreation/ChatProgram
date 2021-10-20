#pragma once

#include "server.h"

class ServerApplication : public ClientHandle {
public:
    ServerApplication() = default;

    bool Init() override;
    bool Handle() override;
};