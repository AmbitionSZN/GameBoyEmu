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


