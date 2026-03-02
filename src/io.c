#include "io.h"
#include "bus.h"
#include "cpu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

extern uint8_t memory[0x10000];

DMA dma;

static uint16_t div = 0xAC00;
static uint8_t *tima = &memory[0xFF05];
static uint8_t *tma = &memory[0xFF06];
static uint8_t *tac = &memory[0xFF07];

uint8_t ioRead(uint16_t address) {
    switch (address) {
    case 0xFF04:
        return (div >> 8);
    case 0xFF44: {
        //  return 0x90;
        uint8_t n = memory[address];
        memory[address] += 1;
        return n;
    }
    default:
        return memory[address];
    }
}

void ioWrite(uint16_t address, uint8_t data) {
    switch (address) {
    case 0xFF04:
        div = 0;
        break;
    case 0xFF46:
        DMAStart(data);
        break;
    default:
        memory[address] = data;
    }
}

void timerTick() {
    uint16_t prevDiv = div;
    div += 1;
    bool timerUpdate = false;

    switch (*tac & 0b11) {
    case 0b00:
        timerUpdate = prevDiv & (1 << 9) && (!(div & (1 << 9)));
        break;
    case 0b01:
        timerUpdate = prevDiv & (1 << 3) && (!(div & (1 << 3)));
        break;
    case 0b10:
        timerUpdate = prevDiv & (1 << 5) && (!(div & (1 << 5)));
        break;
    case 0b11:
        timerUpdate = prevDiv & (1 << 7) && (!(div & (1 << 7)));
        break;
    }

    if (timerUpdate && *tac & (1 << 2)) {
        *tima += 1;

        if (*tima == 0xFF) {
            *tima = *tma;
            requestInterrupt(INT_TIMER);
        }
    }
}

void DMAStart(uint8_t start) {
    dma.Active = true;
    dma.StartDelay = 2;
    dma.Dest = 0xFE00;
    dma.Src = start * 0x100;
}

void DMATransfer(uint16_t dest, uint16_t src) { busWrite(dest, busRead(src)); }

void DMATick() {
    if (!dma.Active) {
        return;
    }

    if (dma.StartDelay) {
        dma.StartDelay--;
        return;
    }

    DMATransfer(dma.Dest, dma.Src);

    dma.Dest++;

    dma.Active = dma.Dest < 0xFEA0;

    /*
    if (!dma.Active) {
        printf("DMA DONE!\n");
        sleep(2);
    }
    */
}
