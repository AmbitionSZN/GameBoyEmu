#include "emu.h"
#include "io.h"

extern Emulator emu;

void emuCycles(int cycles) {
    //TODO...
    int n = cycles * 4;

    for (int i=0; i<n; i++) {
        emu.Ticks++;
        timerTick();
    }
}
