#pragma once

#include <cstdint>

typedef struct {
	uint8_t Entry[4];
	uint8_t Logo[0x30];
	uint8_t Title[11];
	uint8_t ManCode[4];
	uint8_t CGBFlag;
	uint8_t NewLicCode[2];
	uint8_t SGBFlag;
	uint8_t CartType;
	uint8_t RomSize;
	uint8_t RamSize;
	uint8_t DestinationCode;
	uint8_t OldLicCode;
	uint8_t MaskRomVerNumber;
	uint8_t HeaderChecksum;
	uint8_t GlobalChecksum[2];
} Cart_Header;
