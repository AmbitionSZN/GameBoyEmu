#include "emu.h"
#include "io.h"
#include "ppu.h"

extern Emulator emu;

void emuCycles(int cycles) {
    for (int i = 0; i < cycles; i++) {
        for (int n = 0; n < 4; n++) {
            emu.Ticks++;
            timerTick();
			ppuTick();
        }
        DMATick();
    }
}
