#include "ppu.h"
#include "cpu.h"
#include "io.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

extern uint8_t memory[0x10000];

Ppu ppu;

static const uint32_t tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555,
                                       0xFF000000};
static const size_t tilesPerColumn = 16;
static const size_t tilesPerRow = 24;
static const size_t linesPerFrame = 154;
static const size_t ticksPerLine = 456;
static const size_t yRes = 144;
static const size_t xRes = 160;

static uint8_t *const lcdc = &memory[0xFF40];
static uint8_t *const lcds = &memory[0xFF41];
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
    lcdInit();
    lcdsModeSet(MODE_OAM);
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
    // An array of tiles stored as pixels, 64 pixels per tile
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
    /*
    #define LCDC_BGW_ENABLE (BIT(lcd_get_context()->lcdc, 0))
    #define LCDC_OBJ_ENABLE (BIT(lcd_get_context()->lcdc, 1))
    #define LCDC_OBJ_HEIGHT (BIT(lcd_get_context()->lcdc, 2) ? 16 : 8)
    #define LCDC_BG_MAP_AREA (BIT(lcd_get_context()->lcdc, 3) ? 0x9C00 : 0x9800)
    #define LCDC_BGW_DATA_AREA (BIT(lcd_get_context()->lcdc, 4) ? 0x8000 :
    0x8800) #define LCDC_WIN_ENABLE (BIT(lcd_get_context()->lcdc, 5)) #define
    LCDC_WIN_MAP_AREA (BIT(lcd_get_context()->lcdc, 6) ? 0x9C00 : 0x9800)
    #define LCDC_LCD_ENABLE (BIT(lcd_get_context()->lcdc, 7))

    #define LCDS_MODE ((lcd_mode)(lcd_get_context()->lcds & 0b11))
    #define LCDS_MODE_SET(mode) { lcd_get_context()->lcds &= ~0b11;
    lcd_get_context()->lcds |= mode; }

    #define LCDS_LYC (BIT(lcd_get_context()->lcds, 2))
    #define LCDS_LYC_SET(b) (BIT_SET(lcd_get_context()->lcds, 2, b))
#define LCDS_STAT_INT(src) (lcd_get_context()->lcds & src)
    */
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
        }
        break;
    case MODE_XFER:
        if (ppu.LineTicks >= 80 + 172) {
            lcdsModeSet(MODE_HBLANK);
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

                static uint32_t targetFrameTime = 1000 / 60;
                static long prevFrameTime = 0;
                static long startTimer = 0;
                static long frameCount = 0;
                uint32_t end = SDL_GetTicks();
                uint32_t frameTime = end - prevFrameTime;

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

void updatePalette(uint8_t palette_data, uint8_t pal) {
    uint32_t *pColors = bgColors;

    switch (pal) {
    case 1:
        pColors = sp1Colors;
        break;
    case 2:
        pColors = sp2Colors;
        break;
    }

    pColors[0] = colorsDefault[palette_data & 0b11];
    pColors[1] = colorsDefault[(palette_data >> 2) & 0b11];
    pColors[2] = colorsDefault[(palette_data >> 4) & 0b11];
    pColors[3] = colorsDefault[(palette_data >> 6) & 0b11];
}
