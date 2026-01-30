#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool Paused;
    bool Running;
    bool Die;
    uint64_t Ticks;
} Emulator;

void emuCycles(int cycles);
