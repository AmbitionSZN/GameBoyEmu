#pragma once

#include "cart.h"
#include "cpu.h"




void JP(CPU *cpu, Cartridge *cart);
void DI(CPU *cpu);
void XOR(CPU *cpu, Cartridge *cart);
void LD(CPU *cpu, Cartridge *cart);
