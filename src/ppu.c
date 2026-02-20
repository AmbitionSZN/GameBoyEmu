#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t memory[0x10000];
static unsigned long tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void renderTileData(SDL_Renderer* renderer) {
	SDL_FRect rect;
    for (size_t i = 0x8000; i < 0x9800;) {
		uint16_t tile[8];
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte1 = memory[i];
            uint8_t byte2 = memory[i + 1];
			for (size_t k = 7; k > 0; k--) {
				uint8_t loBit = (byte1 & (1 << k));
				uint8_t hiBit = (byte2 & (1 << k));
				unsigned long color = tileColors[loBit | (hiBit << 1)];
			}
            i++;
        }
		SDL_RenderRect(renderer, &rect);
    }
}
