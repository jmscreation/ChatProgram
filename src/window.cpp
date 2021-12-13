#include "window.h"

Window::Window(T_Update updateFunction, T_Create createFunction, uint32_t w, uint32_t h, uint32_t px, uint32_t py, bool fullscreen, bool vsync):
    updateFunction(updateFunction), createFunction(createFunction) {
        sAppName = "Chat Program";

        Construct(w, h, px, py, fullscreen, vsync);
    }

bool Window::OnUserCreate() {
    return createFunction(this);
}

bool Window::OnUserUpdate(float elapsedTime)  { return updateFunction(this, elapsedTime); }

bool Window::OnUserDestroy() {
    return true;
}