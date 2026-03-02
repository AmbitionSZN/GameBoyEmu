#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool Active;
    uint16_t Dest;
    uint16_t Src;
    uint8_t StartDelay;
} DMA;

uint8_t ioRead(uint16_t address);
void ioWrite(uint16_t address, uint8_t data);
void DMATransfer(uint16_t dest, uint16_t src);
void DMAStart(uint8_t start);
void timerTick();
void DMATick();
