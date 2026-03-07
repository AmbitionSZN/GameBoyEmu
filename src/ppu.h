#pragma once

#include <SDL3/SDL_render.h>
#include <stdint.h>

typedef enum {
    FS_GET_TILE,
    FS_DATA_LOW,
    FS_DATA_HIGH,
    FS_SLEEP,
    FS_PUSH
} FetchState;

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
    FetchState State;
    uint8_t LineX;
    uint8_t PushedX;
    uint8_t FetchX;
    uint8_t BgWFetchData[3];
    uint8_t FetchEntryData[6]; // oam data..
    uint8_t MapY;
    uint8_t MapX;
    uint8_t TileY;
    uint8_t FifoX;
} PixelFetcher;

typedef struct {
    uint8_t Y;
    uint8_t X;
    uint8_t TileIdx;
    uint8_t Attributes;
} ObjAttribute;

typedef struct _OamLineEntry {
	ObjAttribute Obj;
	struct _OamLineEntry *Next;
} OamLineEntry;

typedef struct {
    uint32_t CurrentFrame;
    uint32_t LineTicks;
	OamLineEntry *LineSprites;
	OamLineEntry LineEntryArray[10];
	ObjAttribute FetchedEntries[3];
	uint8_t LineSpriteCount;
	uint8_t FetchedEntryCount; 
} Ppu;

typedef enum { MODE_HBLANK, MODE_VBLANK, MODE_OAM, MODE_XFER } LcdMode;

typedef enum {
    LCDC_BGW_ENABLE = 1,
    LCDC_OBJ_ENABLE = (1 << 1),
    LCDC_OBJ_SIZE = (1 << 2),
    LCDC_BG_TILE_MAP = (1 << 3),
    LCDC_BGW_TILE_DATA = (1 << 4),
    LCDC_WINDOW_ENABLE = (1 << 5),
    LCDC_WINDOW_TILE_MAP = (1 << 6),
    LCDC_LCD_ENABLE = (1 << 7)
} LcdcFlag;

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
} StatSrc;

void renderTile(SDL_Renderer *renderer, int winW, int winH, int x, int y,
                uint8_t *tileData);
void renderTiles(SDL_Renderer *renderer, int winW, int winH);
void render(SDL_Renderer *render);
void lcdInit();
void ppuTick();
void updatePalette(uint8_t palette_data, uint8_t pal);
void fifoPush(uint32_t color);
uint32_t fifoPop();
void fifoReset();
void pushPixel();
void pixelProcess();
