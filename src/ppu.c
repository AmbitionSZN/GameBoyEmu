#include "ppu.h"
#include "bus.h"
#include "cpu.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t memory[0x10000];

Ppu ppu;
static const size_t yRes = 144;
static const size_t xRes = 160;
static uint32_t videoBuffer[144 * 160];
PixelFifo fifo;
PixelFetcher pxFetcher;

static const uint32_t tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555,
                                       0xFF000000};
static const size_t tilesPerColumn = 16;
static const size_t tilesPerRow = 24;
static const size_t linesPerFrame = 154;
static const size_t ticksPerLine = 456;

static uint8_t *const lcdc = &memory[0xFF40];
static uint8_t *const lcds = &memory[0xFF41];
static uint8_t *const scrollY = &memory[0xFF42];
static uint8_t *const scrollX = &memory[0xFF43];
static uint8_t *const ly = &memory[0xFF44];
static uint8_t *const lyc = &memory[0xFF45];
static uint8_t *const bgPalette = &memory[0xFF47];
static uint8_t *const objP0 = &memory[0xFF48];
static uint8_t *const objP1 = &memory[0xFF49];
static const uint32_t colorsDefault[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555,
                                          0xFF000000};
static uint32_t bgColors[4];
static uint32_t sp1Colors[4];
static uint32_t sp2Colors[4];

void lcdsModeSet(LcdMode mode) {
    *lcds &= ~0b11;
    *lcds |= mode;
}

void ppuInit() {
    ppu.CurrentFrame = 0;
    ppu.LineTicks = 0;
    ppu.LineSprites = 0;
    ppu.FetchedEntryCount = 0;

    lcdInit();
    lcdsModeSet(MODE_OAM);
    pxFetcher.State = FS_GET_TILE;
    pxFetcher.LineX = 0;
    pxFetcher.PushedX = 0;
    pxFetcher.FetchX = 0;
    fifo.Size = 0;
}

void loadLineSprites() {
    int curY = *ly;

    uint8_t spriteSize = (*lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    memset(ppu.LineEntryArray, 0, sizeof(ppu.LineEntryArray));

    for (size_t i = 0; i < 40; i++) {
        ObjAttribute obj;
        memcpy(&obj, &memory[0xFE00 + (i * sizeof(ObjAttribute))],
               sizeof(ObjAttribute));

        if (!obj.X) {
            // x = 0 means not visible...
            continue;
        }

        if (ppu.LineSpriteCount >= 10) {
            // max 10 sprites per line...
            break;
        }

        if (obj.Y <= curY + 16 && obj.Y + spriteSize > curY + 16) {
            // this sprite is on the current line.

            OamLineEntry *entry = &ppu.LineEntryArray[ppu.LineSpriteCount++];

            entry->Obj = obj;
            entry->Next = NULL;

            if (!ppu.LineSprites || ppu.LineSprites->Obj.X > obj.X) {
                entry->Next = ppu.LineSprites;
                ppu.LineSprites = entry;
                continue;
            }

            // do some sorting...

            OamLineEntry *le = ppu.LineSprites;
            OamLineEntry *prev = le;

            while (le) {
                if (le->Obj.X > obj.X) {
                    prev->Next = entry;
                    entry->Next = le;
                    break;
                }

                if (!le->Next) {
                    le->Next = entry;
                    break;
                }

                prev = le;
                le = le->Next;
            }
        }
    }
}

uint32_t fetchSpritePixels(int bit, uint32_t color, uint8_t bgColor) {
    for (uint8_t i = 0; i < ppu.FetchedEntryCount; i++) {
        int spriteX = (ppu.FetchedEntries[i].X - 8) + ((*scrollX % 8));

        if (spriteX + 8 < pxFetcher.FifoX) {
            // past pixel point already...
            continue;
        }

        int offset = pxFetcher.FifoX - spriteX;

        if (offset < 0 || offset > 7) {
            // out of bounds..
            continue;
        }

        bit = (7 - offset);

        if (ppu.FetchedEntries[i].Attributes & (1 << 5)) {
            bit = offset;
        }

        uint8_t lo = !!(pxFetcher.FetchEntryData[i * 2] & (1 << bit));
        uint8_t hi = !!(pxFetcher.FetchEntryData[(i * 2) + 1] & (1 << bit))
                     << 1;

        bool bgPriority = ppu.FetchedEntries[i].Attributes & (1 << 7);

        if (!(hi | lo)) {
            // transparent
            continue;
        }

        if (!bgPriority || bgColor == 0) {
            color = (ppu.FetchedEntries[i].Attributes & (1 << 4))
                        ? sp2Colors[hi | lo]
                        : sp1Colors[hi | lo];

            if (hi | lo) {
                break;
            }
        }
    }

    return color;
}

void loadSpriteTile() {
    OamLineEntry *le = ppu.LineSprites;

    while (le) {
        int spriteX = (le->Obj.X - 8) + (*scrollX % 8);

        if ((spriteX >= pxFetcher.FetchX && spriteX < pxFetcher.FetchX + 8) ||
            ((spriteX + 8) >= pxFetcher.FetchX &&
             (spriteX + 8) < pxFetcher.FetchX + 8)) {
            // need to add entry
            ppu.FetchedEntries[ppu.FetchedEntryCount++] = le->Obj;
        }

        le = le->Next;

        if (!le || ppu.FetchedEntryCount >= 3) {
            // max checking 3 sprites on pixels
            break;
        }
    }
}

void loadSpriteData(int offset) {
    int curY = *ly;
    uint8_t spriteSize = (*lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (int i = 0; i < ppu.FetchedEntryCount; i++) {
        uint8_t tileY = ((curY + 16) - ppu.FetchedEntries[i].Y) * 2;

        if (ppu.FetchedEntries[i].Attributes & (1 << 6)) {
            // flipped upside down...
            tileY = ((spriteSize * 2) - 2) - tileY;
        }

        uint8_t tileIndex = ppu.FetchedEntries->TileIdx;

        if (spriteSize == 16) {
            tileIndex &= ~1;
        }

        pxFetcher.FetchEntryData[(i * 2) + offset] =
            busRead(0x8000 + (tileIndex * 16) + tileY + offset);
    }
}

void renderTile(SDL_Renderer *renderer, int winW, int winH, int x, int y,
                uint8_t *tileData) {
    for (size_t byte = 0; byte < 16;) {
        int row = byte / 2;
        uint8_t byte1 = tileData[byte];
        byte++;
        uint8_t byte2 = tileData[byte];
        byte++;
        for (int pxIdx = 7; pxIdx >= 0; pxIdx--) {
            uint8_t loBit = !!(byte1 & (1 << pxIdx));
            uint8_t hiBit = !!(byte2 & (1 << pxIdx));
            uint32_t color = tileColors[loBit | (hiBit << 1)];
            SDL_FRect pixel;
            pixel.w = ((float)winW / tilesPerRow) / 8;
            pixel.h = ((float)winH / tilesPerColumn) / 8;
            pixel.y = y + (row * pixel.h);
            pixel.x = x + (pixel.w * (7 - pxIdx));
            uint8_t r = (color & (0xFF << 4)) >> 4;
            uint8_t g = (color & (0xFF << 2)) >> 2;
            uint8_t b = (color & 0xFF);
            SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &pixel);
        }
    }
}

void renderTiles(SDL_Renderer *renderer, int winW, int winH) {
    size_t curTile = 0;
    size_t tileWidth = winW / tilesPerRow;
    size_t tileHeight = winH / tilesPerColumn;

    for (size_t i = 0x8000; i < 0x9800;) {
        size_t row = curTile / tilesPerRow;
        size_t col = curTile % tilesPerRow;
        int y = row * tileHeight;
        int x = col * tileWidth;
        curTile++;
        uint8_t *tile = &memory[i];
        renderTile(renderer, winW, winH, x, y, tile);
        i += 16;
    }
}

void render(SDL_Renderer *renderer) {
    SDL_FRect rc;
    rc.x = rc.y = 0;
    rc.w = rc.h = 2048;
    int scale = 4;

    for (size_t lineNum = 0; lineNum < yRes; lineNum++) {
        for (size_t x = 0; x < xRes; x++) {
            rc.x = x * scale;
            rc.y = lineNum * scale;
            rc.w = scale;
            rc.h = scale;
            uint32_t color = videoBuffer[x + (lineNum * xRes)];
            uint8_t r = (color & (0xFF << 4)) >> 4;
            uint8_t g = (color & (0xFF << 2)) >> 2;
            uint8_t b = (color & 0xFF);

            SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &rc);
        }
    }
}

void lcdInit() {
    *lcdc = 0x91;
    *bgPalette = 0xFC;
    *objP0 = 0xFF;
    *objP1 = 0xFF;

    for (int i = 0; i < 4; i++) {
        bgColors[i] = colorsDefault[i];
        sp1Colors[i] = colorsDefault[i];
        sp2Colors[i] = colorsDefault[i];
    }
}

void incrementLy() {
    *ly += 1;

    if (*ly == *lyc) {
        *lcds |= (1 << 2);

        if (*lcds & SS_LYC) {
            requestInterrupt(INT_LCD);
        }

    } else {
        *lcds &= ~(1 << 2);
    }
}

void ppuTick() {
    ppu.LineTicks++;

    switch (*lcds & 0b11) {
    case MODE_OAM:
        if (ppu.LineTicks >= 80) {
            lcdsModeSet(MODE_XFER);
            pxFetcher.State = FS_GET_TILE;
            pxFetcher.LineX = 0;
            pxFetcher.FetchX = 0;
            pxFetcher.PushedX = 0;
            pxFetcher.FifoX = 0;

            if (ppu.LineTicks == 1) {
                ppu.LineSprites = 0;
                ppu.LineSpriteCount = 0;
                loadLineSprites();
            }
        }
        break;
    case MODE_XFER:
        pixelProcess();
        if (pxFetcher.PushedX >= xRes) {
            fifoReset();
            lcdsModeSet(MODE_HBLANK);
            if (*lcds & SS_HBLANK) {
                requestInterrupt(INT_LCD);
            }
        }
        break;
    case MODE_VBLANK:
        if (ppu.LineTicks >= ticksPerLine) {
            incrementLy();
            if (*ly >= linesPerFrame) {
                lcdsModeSet(MODE_OAM);
                *ly = 0;
            }
            ppu.LineTicks = 0;
        }
        break;
    case MODE_HBLANK:
        if (ppu.LineTicks >= ticksPerLine) {
            incrementLy();

            if (*ly >= yRes) {
                lcdsModeSet(MODE_VBLANK);
                requestInterrupt(INT_VBLANK);

                if (*lcds & SS_VBLANK) {
                    requestInterrupt(INT_LCD);
                }

                ppu.CurrentFrame++;

                static const size_t targetFrameTime = 1000 / 60;
                static size_t prevFrameTime = 0;
                static size_t startTimer = 0;
                static size_t frameCount = 0;
                size_t end = SDL_GetTicks();
                size_t frameTime = end - prevFrameTime;

                if (frameTime < targetFrameTime) {
                    SDL_Delay((targetFrameTime - frameTime));
                }

                if (end - startTimer >= 1000) {
                    uint32_t fps = frameCount;
                    startTimer = end;
                    frameCount = 0;

                    printf("FPS: %d\n", fps);
                }

                frameCount++;
                prevFrameTime = SDL_GetTicks();
            } else {
                lcdsModeSet(MODE_OAM);
            }
            ppu.LineTicks = 0;
        }
        break;
    }
}

void updatePalette(uint8_t paletteData, uint8_t pal) {
    uint32_t *pColors = bgColors;

    switch (pal) {
    case 1:
        pColors = sp1Colors;
        break;
    case 2:
        pColors = sp2Colors;
        break;
    }

    pColors[0] = colorsDefault[paletteData & 0b11];
    pColors[1] = colorsDefault[(paletteData >> 2) & 0b11];
    pColors[2] = colorsDefault[(paletteData >> 4) & 0b11];
    pColors[3] = colorsDefault[(paletteData >> 6) & 0b11];
}

void fifoPush(uint32_t color) {
    PxFifoEntry *entry = malloc(sizeof(PxFifoEntry));
    entry->Color = color;
    entry->Next = NULL;
    if (fifo.Size == 0) {
        fifo.Head = entry;
    } else {
        fifo.Tail->Next = entry;
    }
    fifo.Tail = entry;
    fifo.Size++;
}

uint32_t fifoPop() {
    assert(fifo.Size);

    uint32_t color = fifo.Head->Color;
    PxFifoEntry *next = fifo.Head->Next;
    free(fifo.Head);

    if (fifo.Size == 1) {
        fifo.Tail = NULL;
    }

    fifo.Head = next;
    fifo.Size--;
    return color;
}

void fifoReset() {
    while (fifo.Size) {
        fifoPop();
    }

    fifo.Head = NULL;
}

void pixelFetch() {
    /*
#define LCDC_BGW_ENABLE (BIT(lcd_get_context()->lcdc, 0))
#define LCDC_OBJ_ENABLE (BIT(lcd_get_context()->lcdc, 1))
#define LCDC_OBJ_HEIGHT (BIT(lcd_get_context()->lcdc, 2) ? 16 : 8)
#define LCDC_BG_MAP_AREA (BIT(lcd_get_context()->lcdc, 3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_DATA_AREA (BIT(lcd_get_context()->lcdc, 4) ? 0x8000 : 0x8800)
#define LCDC_WIN_ENABLE (BIT(lcd_get_context()->lcdc, 5))
#define LCDC_WIN_MAP_AREA (BIT(lcd_get_context()->lcdc, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_ENABLE (BIT(lcd_get_context()->lcdc, 7))

#define LCDS_MODE ((lcd_mode)(lcd_get_context()->lcds & 0b11))
#define LCDS_MODE_SET(mode) { lcd_get_context()->lcds &= ~0b11;
lcd_get_context()->lcds |= mode; }

#define LCDS_LYC (BIT(lcd_get_context()->lcds, 2))
#define LCDS_LYC_SET(b) (BIT_SET(lcd_get_context()->lcds, 2, b))

    */
    switch (pxFetcher.State) {
    case FS_GET_TILE:
        ppu.FetchedEntryCount = 0;
        if (*lcdc & LCDC_BGW_ENABLE) {
            uint8_t *tileMap =
                &memory[(*lcdc & LCDC_BG_TILE_MAP) ? 0x9C00 : 0x9800];
            pxFetcher.BgWFetchData[0] =
                tileMap[(pxFetcher.MapX / 8) + ((pxFetcher.MapY / 8) * 32)];

            if (((*lcdc & LCDC_BGW_TILE_DATA) ? 0x8000 : 0x8800) == 0x8800) {
                pxFetcher.BgWFetchData[0] += 128;
            }
        }
        if ((*lcdc & LCDC_OBJ_ENABLE) && ppu.LineSprites) {
            loadSpriteTile();
        }
        pxFetcher.State = FS_DATA_LOW;
        pxFetcher.FetchX += 8;
        break;
    case FS_DATA_LOW: {
        uint8_t *dataArea =
            &memory[(*lcdc & LCDC_BGW_TILE_DATA) ? 0x8000 : 0x8800];
        pxFetcher.BgWFetchData[1] =
            dataArea[(pxFetcher.BgWFetchData[0] * 16) + pxFetcher.TileY];
        loadSpriteData(0);
        pxFetcher.State = FS_DATA_HIGH;
        break;
    }
    case FS_DATA_HIGH: {
        uint8_t *dataArea =
            &memory[(*lcdc & LCDC_BGW_TILE_DATA) ? 0x8000 : 0x8800];
        pxFetcher.BgWFetchData[2] =
            dataArea[(pxFetcher.BgWFetchData[0] * 16) + pxFetcher.TileY + 1];
        loadSpriteData(1);
        pxFetcher.State = FS_SLEEP;
        break;
    }
    case FS_SLEEP:
        pxFetcher.State = FS_PUSH;
        break;
    case FS_PUSH:

        if (fifo.Size > 8) {
            // fifo is full!
            break;
        }

        int x = pxFetcher.FetchX - (8 - (*scrollX % 8));

        for (int i = 0; i < 8; i++) {
            int bit = 7 - i;
            uint8_t lo = !!(pxFetcher.BgWFetchData[1] & (1 << bit));
            uint8_t hi = !!(pxFetcher.BgWFetchData[2] & (1 << bit)) << 1;
            uint32_t color = bgColors[hi | lo];

            if (!(*lcdc & LCDC_BGW_ENABLE)) {
                color = bgColors[0];
            }

            if (*lcdc & LCDC_OBJ_ENABLE) {
                color = fetchSpritePixels(bit, color, hi | lo);
            }

            if (x >= 0) {
                fifoPush(color);
                pxFetcher.FifoX++;
            }
        }

        pxFetcher.State = FS_GET_TILE;
        break;
    }
}

void pushPixel() {
    if (fifo.Size > 8) {
        uint32_t pixelData = fifoPop();

        if (pxFetcher.LineX >= (*scrollX % 8)) {
            videoBuffer[pxFetcher.PushedX + (*ly * xRes)] = pixelData;
            pxFetcher.PushedX++;
        }

        pxFetcher.LineX++;
    }
}

void pixelProcess() {
    pxFetcher.MapY = *ly + *scrollY;
    pxFetcher.MapX = pxFetcher.FetchX + *scrollX;
    pxFetcher.TileY = ((*ly + *scrollY) % 8) * 2;

    if (!(ppu.LineTicks & 1)) {
        pixelFetch();
    }
    pushPixel();
}
