#include "pause.h"



void pause() {
    std::cout << "Press any key to continue...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    do {

        if(GetForegroundWindow() == GetConsoleWindow()){
            for(int i=0x05; i < 0xFF; ++i){
                if( GetAsyncKeyState(i) & 0x8000 ) return;
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    } while(1);
}