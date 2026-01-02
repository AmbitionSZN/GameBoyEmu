#include "cart.h"
#include <stdio.h>
#include <stdlib.h>

void LoadCartridge(char *file) {
    Cartridge cart;
    FILE *fptr;
    fptr = fopen(file, "r");
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
    printf("\t Type     : %X\n", cart.Header->Type);
    printf("\t ROM Size : %u\n", cart.Header->RomSize);
    printf("\t RAM Size : %2.2X\n", cart.Header->RamSize);
    printf("\t LIC Code : %X\n", cart.Header->OldLicCode);
    printf("\t ROM Vers : %2.2X\n", cart.Header->Version);
}
