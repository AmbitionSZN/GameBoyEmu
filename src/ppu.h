#pragma once

#include <SDL3/SDL_render.h>


void renderTile(SDL_Renderer *renderer, int winW, int winH, int x, int y, uint8_t *tileData);
void renderTiles(SDL_Renderer *renderer, int winW, int winH);
