#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t memory[0x10000];
static unsigned long tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555,
                                      0xFF000000};
static int tilesPerColumn = 16;
static int tilesPerRow = 24;

SDL_Surface *getTileSurfaces(int winW, int winH) {
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
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte1 = memory[i];
            uint8_t byte2 = memory[i + 1];
            for (size_t k = 7; k > 0; k--) {
                uint8_t loBit = (byte1 & (1 << k));
                uint8_t hiBit = (byte2 & (1 << k));
                unsigned long color = tileColors[loBit | (hiBit << 1)];
                SDL_Rect r;
                r.w = (winW / tilesPerRow) / 8;
                r.h = (winH / tilesPerColumn) / 8;
				r.y = (row * colSize) + (r.h * k);
				r.x = (col * rowSize) + (r.w * k);
                SDL_FillSurfaceRect(&tiles[pixel], &r, color);
                pixel++;
            }
            i += 2;
        }
    }
    return &tiles[0];
}

void renderTiles(SDL_Surface *tiles, int winW, int winH) {
    int colSize = winH / tilesPerColumn;
    int rowSize = winW / tilesPerRow;
    for (size_t i = 0; i < 384; i++) {
        tiles[i]
    }
    for (size_t col = 0; col < tilesPerColumn; col++) {
        int y = colSize * col;
        for (size_t row = 0; row < tilesPerRow; row++) {
            int x = rowSize * row;
        }
    }
}
