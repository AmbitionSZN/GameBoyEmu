#pragma once

#include <stdint.h>

typedef struct {
	uint8_t Entry[4];
	uint8_t Logo[0x30];
	char Title[16];
	uint16_t NewLicCode;
	uint8_t SGBFlag;
	uint8_t Type;
	uint8_t RomSize;
	uint8_t RamSize;
	uint8_t DestinationCode;
	uint8_t OldLicCode;
	uint8_t Version;
	uint8_t HeaderChecksum;
	uint16_t GlobalChecksum;
	
} CartHeader;

typedef struct {
	uint32_t RomSize;
	uint8_t *RomData;
	CartHeader *Header;
} Cartridge;


Cartridge LoadCartridge(char *file);

uint8_t cartRead(uint16_t);


