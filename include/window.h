#pragma once

#include "pge.h"
#include <functional>

class Window; //forward declarator

typedef std::function<bool (Window*, float)> T_Update;
typedef std::function<bool (Window*)> T_Create;

class Window : public olc::PixelGameEngine {
    T_Update updateFunction;
    T_Create createFunction;
public:
    Window(T_Update updateFunction, T_Create createFunction, uint32_t w, uint32_t h, uint32_t px = 1, uint32_t py = 1, bool fullscreen=false, bool vsync=true);
    virtual ~Window() = default;
public:
    bool OnUserCreate() override;
    
    bool OnUserUpdate(float elapsedTime) override;
    
    bool OnUserDestroy() override;
};