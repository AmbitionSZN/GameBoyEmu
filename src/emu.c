#include "emu.h"
#include "io.h"
#include "ppu.h"

extern Emulator emu;
extern uint8_t cyclesTaken;

void emuCycles(int cycles) {
	cyclesTaken++;
    for (int i = 0; i < cycles; i++) {
        for (int n = 0; n < 4; n++) {
            emu.Ticks++;
            timerTick();
			ppuTick();
        }
        DMATick();
    }
}
