#include "cart.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t memory[0x10000];
extern Cartridge cart;

Cartridge LoadCartridge(char *file) {
    Cartridge cart;
    FILE *fptr;
    fptr = fopen(file, "rb");
    if (fptr == NULL) {
        printf("failed to open rom: %s", file);
        exit(EXIT_FAILURE);
    }
    fseek(fptr, 0, SEEK_END);
    cart.RomSize = ftell(fptr);
    rewind(fptr);
    cart.RomData = malloc(cart.RomSize);
    fread(cart.RomData, cart.RomSize, 1, fptr);
    fclose(fptr);
    cart.Header = (CartHeader *) (cart.RomData + 0x100);
    cart.Header->Title[15] = 0;

    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", cart.Header->Title);
    printf("\t Type     : %2.2X\n", cart.Header->Type);
    printf("\t ROM Size : %u KB\n", 32 << cart.Header->RomSize);
    printf("\t RAM Size : %2.2X\n", cart.Header->RamSize);
    printf("\t LIC Code : %X\n", cart.Header->OldLicCode);
    printf("\t ROM Vers : %2.2X\n", cart.Header->Version);
	return cart;
}

uint8_t cartRead(uint16_t address) {
	return cart.RomData[address];	
}

void cartWrite(uint16_t address, uint8_t val) {
	printf("attempt to write to cart\naddress: %X", address);
	return;
}
