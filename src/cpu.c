#include "cpu.h"
#include "bus.h"
#include "cart.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

Instruction Instructions[512];

void opcodesJsonParser(char *file) {
    FILE *fptr;
	char *json;
	size_t jsonSize;
    fptr = fopen(file, "rb");
    if (fptr == NULL) {
        printf("failed to open rom: %s", file);
        exit(EXIT_FAILURE);
    }
    fseek(fptr, 0, SEEK_END);
    jsonSize = ftell(fptr);
    rewind(fptr);
    json = malloc(jsonSize);
    fread(json, jsonSize, 1, fptr);
    fclose(fptr);

	for (size_t i = 0; i < 100; i++) {
		printf("%s", &json[i]);
	}
};

void jump(CPU *cpu, Cartridge cart) {
    uint16_t pc;
    uint16_t lo = cart.RomData[cpu->Regs.PC + 1];
    uint16_t hi = cart.RomData[cpu->Regs.PC + 2];
    pc = lo | (hi << 8);
    printf("pc: %2.2X", pc);
}

void fetchInstruction(CPU *cpu, Cartridge cart) {
    uint8_t opcode = busRead(cpu->Regs.PC, cart);

    jump(cpu, cart);
}
