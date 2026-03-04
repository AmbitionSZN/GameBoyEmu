#pragma once

#include <SDL3/SDL_render.h>
#include <stdint.h>

typedef struct _PxFifoEntry {
	uint32_t Color;
	struct _PxFifoEntry *Next;
} PxFifoEntry;

typedef struct {
	PxFifoEntry *Head;
	PxFifoEntry *Tail;
	size_t Size;
} PixelFifo;

typedef struct {
	uint8_t Y;
	uint8_t X;
	uint8_t TileIdx;
	uint8_t Attributes;

} ObjAttribute;

typedef struct {
	uint32_t CurrentFrame;
	uint32_t LineTicks;
} Ppu;

typedef enum {
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_XFER
} LcdMode;

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
} StatSrc;



void renderTile(SDL_Renderer *renderer, int winW, int winH, int x, int y, uint8_t *tileData);
void renderTiles(SDL_Renderer *renderer, int winW, int winH);
void lcdInit();
void ppuTick();
void updatePalette(uint8_t palette_data, uint8_t pal);
void fifoPush(uint32_t color);
uint32_t fifoPop();

