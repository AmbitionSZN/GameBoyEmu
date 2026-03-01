#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t memory[0x10000];
static uint32_t tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555,
                                 0xFF000000};
static size_t tilesPerColumn = 16;
static size_t tilesPerRow = 24;

void renderTiles(SDL_Renderer *renderer, int winW, int winH) {
    // An array of tiles stored as pixels, 64 pixels per tile
    static SDL_Surface tiles[0x6000];
    size_t pixel = 0;
    size_t curTile = 0;
    size_t colSize = winH / tilesPerColumn;
    size_t rowSize = winW / tilesPerRow;

    for (size_t i = 0x8000; i < 0x9800;) {
        size_t row = curTile / tilesPerRow;
        size_t col = curTile % tilesPerRow;
        curTile++;
        uint8_t testTile[16] = {0x7C, 0x7C, 0x00, 0xC6, 0xC6, 0x00, 0x00, 0xFE,
                                0xC6, 0xC6, 0x00, 0xC6, 0xC6, 0x00, 0x00, 0x00};
        for (size_t j = 0; j < 8; j += 2) {
            uint8_t byte1 = testTile[j];
            uint8_t byte2 = testTile[j + 1];
            for (int k = 7; k >= 0; k--) {
                uint8_t loBit = (byte1 & (1 << k));
                uint8_t hiBit = (byte2 & (1 << k));
                uint32_t color = tileColors[loBit | (hiBit << 1)];
                SDL_FRect rect;
                rect.w = ((float)winW / tilesPerRow) / 8;
                rect.h = ((float)winH / tilesPerColumn) / 8;
                rect.y = (row * colSize);
                rect.x = (col * rowSize) + (rect.w * k);
                uint8_t r = (color & (0xFF << 4)) >> 4;
                uint8_t g = (color & (0xFF << 2)) >> 2;
                uint8_t b = (color & 0xFF);
                SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &rect);
                pixel++;
            }
            i += 2;
        }
    }
}

void renderTile(SDL_Renderer *renderer, int winW, int winH, int x, int y, uint8_t *tileData) {
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
